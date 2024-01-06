#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <signal.h>

#include "common/constants.h"
#include "common/io.h"
#include "operations.h"
#include "threads.h"

// Extern variable so that the signal handler and produce function can change it
extern int sig_flag;

int main(int argc, char* argv[]) {
  if (signal(SIGUSR1, sig_handler) == SIG_ERR) {
    fprintf(stderr, "[ERR]: Failed to change how SIGUSR1 is handled\n");
  }

  if (argc < 2 || argc > 3) {
    fprintf(stderr, "Usage: %s\n <pipe_path> [delay]\n", argv[0]);
    return 1;
  }

  char* endptr;
  unsigned int state_access_delay_us = STATE_ACCESS_DELAY_US;
  if (argc == 3) {
    unsigned long int delay = strtoul(argv[2], &endptr, 10);

    if (*endptr != '\0' || delay > UINT_MAX) {
      fprintf(stderr, "Invalid delay value or value too large\n");
      return 1;
    }

    state_access_delay_us = (unsigned int)delay;
  }

  if (ems_init(state_access_delay_us)) {
    fprintf(stderr, "Failed to initialize EMS\n");
    return 1;
  }

  char server_pathname[PIPE_PERMS];
  strncpy(server_pathname, argv[1], PIPE_NAME_SIZE);

  // Blocks the SIGPIPE signal to avoid terminating when a client quits mid session
  sigset_t set;
  sigemptyset(&set);
  sigaddset(&set, SIGPIPE);
  if (pthread_sigmask(SIG_BLOCK, &set, NULL) != 0) {
    fprintf(stderr, "[ERR]: Failed masking SIGPIPE\n");
  };
  
  // producer-consumer buffer
  client_args prodConsBuf[MAX_SESSION_COUNT];
  memset(prodConsBuf, 0, sizeof(prodConsBuf));

  int prodptr = 0, consptr = 0, count = 0;

  // mutex and conditional variables initialization
  pthread_mutex_t mutex;
  if (pthread_mutex_init(&mutex, NULL) != 0) {
    fprintf(stderr, "[ERR]: Failed do initialize mutex\n");
  }
  pthread_cond_t podeProd;
  if (pthread_cond_init(&podeProd, NULL) != 0) {
    fprintf(stderr, "[ERR]: Failed do initialize conditional variable\n");
  }
  pthread_cond_t podeCons;
  if (pthread_cond_init(&podeCons, NULL) != 0) {
    fprintf(stderr, "[ERR]: Failed do initialize conditional variable\n");
  }

  open_pipe(server_pathname, PIPE_PERMS);
  // Opened pipe for rdwr to avoid the pipe closing
  int server_fd = safe_open(server_pathname, O_RDWR);

  pthread_t worker_threads[MAX_SESSION_COUNT];

  for (int i = 0; i < MAX_SESSION_COUNT; i++) {
    worker_args *args = (worker_args*) safe_malloc(sizeof(worker_args));
    args->prodConsBuf = prodConsBuf;
    args->consptr = &consptr;
    args->prodptr = &prodptr;
    args->count = &count;
    args->session_id = i;
    args->mutex = &mutex;
    args->podeCons = &podeCons;
    args->podeProd = &podeProd;
    if (pthread_create(&worker_threads[i], NULL, worker_thread_func, args) != 0) {
      fprintf(stderr, "[ERR]: Failed to create worker thread\n");
    }
  }

  // Server stays in an indefinite loop where it is either waiting for a client to start
  // a session to fetch its pipes or for a worker thread to consume one entry of the buffer
  while (1) {
    client_args item = produce(server_fd);
    pthread_mutex_lock(&mutex);
    while (count == MAX_SESSION_COUNT) {
      if (sig_flag) {
        if (sig_flag == 2) fprintf(stderr, "[ERR]: Failed to change how SIGUSR1 is handled\n");
        ems_list_and_show();
        sig_flag = 0;
      }
    pthread_cond_wait(&podeProd, &mutex);
    }
    prodConsBuf[prodptr++] = item;
    if (prodptr == MAX_SESSION_COUNT) {
      prodptr = 0;
    }
    count++;
    pthread_cond_signal(&podeCons);
    pthread_mutex_unlock(&mutex);
  }

  // it should not get this far
  return ems_terminate();
}