#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

#include "common/constants.h"
#include "common/io.h"
#include "operations.h"

void *worker_thread_func(void *rwlock) {
  // TODO
}

void *main_thread_func(void *pathname) {
  char *server_pathname = (char*) pathname;

  if (unlink(server_pathname) != 0 && errno != ENOENT) {
    fprintf(stderr, "unlink(%s) failed: %s\n", server_pathname, strerror(errno));
    exit(EXIT_FAILURE);
  }
  fprintf(stderr, "Server pipe: %s\n", server_pathname);
  if (mkfifo(server_pathname, SERVER_PIPE) != 0) {
    fprintf(stderr, "mkfifo failed: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  // TODO: Multi-threaded server
  pthread_t worker_thread;
  if (pthread_create(&worker_thread, NULL, worker_thread_func, NULL) != 0) {
    fprintf(stderr, "Failed to create worker thread\n");
    return 1;
  }

  

  while (1) { // Copilot
    int server_fd = open(server_pathname, O_RDONLY);
    if (server_fd == -1) {
      fprintf(stderr, "Failed to open server pipe\n");
      return 1;
    }

    char command;
    read_pipe_line(server_fd, &command, sizeof(command));
    printf("Received command: %c\n", command);
  }
  
  return NULL;
}

int main(int argc, char* argv[]) {
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

  //TODO: Intialize server, create worker threads
  char server_pathname[MAX_PIPE_NAME];
  strncpy(server_pathname, argv[1], sizeof(server_pathname));
  
  pthread_t main_thread;
  if (pthread_create(&main_thread, NULL, main_thread_func, server_pathname) != 0) {
    fprintf(stderr, "Failed to create main thread\n");
    return 1;
  } 
  
  // void *ret_value; TODO
  if (pthread_join(main_thread, NULL) != 0) {
    fprintf(stderr, "Failed to join main thread\n");
    return 1;
  }
  
  while (1) {
    //TODO: Read from pipe
    //TODO: Write new client to the producer-consumer buffer
  }

  //TODO: Close Server

  return ems_terminate();
}
