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

int S = 0;
int active_sessions = 0;
int server_fd;
volatile int cond = 0;

typedef struct {
  char req_pipe_path[MAX_PIPE_NAME];
  char resp_pipe_path[MAX_PIPE_NAME];
} client_args;

typedef struct {
  client_args* pcb;
  int *prodptr, *consptr, *count;
  pthread_mutex_t *mutex;
  pthread_cond_t *podeProd, *podeCons;
} temp;

static void sig_handler(int sig) {
  if (sig == SIGUSR1) cond = 1;
  if (signal(SIGUSR1, sig_handler) == SIG_ERR) {
    fprintf(stderr, "TODO\n");
    exit(EXIT_FAILURE);
  }
}

// TODO: change cuz full copied from gpt
ssize_t safe_read2(int fd, void *buf, size_t count) {
    ssize_t bytesRead;

    // Loop until a successful read or an error occurs
    do {
        bytesRead = read(fd, buf, count);

        // Check for errors
        if (bytesRead == -1) {
            // Handle specific errors, if needed
            if (errno == EINTR) {
                // The read was interrupted by a signal, retry
                continue;
            } else {
                // Handle other errors, e.g., print an error message
                perror("Error reading from file descriptor");
                break;
            }
        }
    } while (bytesRead == -1);

    return bytesRead;
}

client_args produz(int fd) {
  client_args ret;
  
  char code = '0';
  while (code != '1') {
    safe_read(fd, &code, sizeof(char));
    if (cond) {
      // TODO
      ems_list_and_show();
      cond = 0;
    }
  }
  // printf("1\n");
  // safe_read(fd, &code, sizeof(char));
  // printf("2\n");
  
  char req_pipe_path[MAX_PIPE_NAME];
  char resp_pipe_path[MAX_PIPE_NAME];

  safe_read2(fd, req_pipe_path, MAX_PIPE_NAME);
  safe_read2(fd, resp_pipe_path, MAX_PIPE_NAME);
  fprintf(stderr, "Request pipe: %s\n", req_pipe_path);
  fprintf(stderr, "Response pipe: %s\n", resp_pipe_path);

  strncpy(ret.req_pipe_path, req_pipe_path, MAX_PIPE_NAME);
  strncpy(ret.resp_pipe_path, resp_pipe_path, MAX_PIPE_NAME);

  return ret;
}

void *worker_thread_func(void *args) {
  sigset_t set;
  sigemptyset(&set);
  sigaddset(&set, SIGUSR1);
  if (pthread_sigmask(SIG_BLOCK, &set, NULL) != 0) {
    fprintf(stderr, "Failed masking the signal\n");
    return NULL;
  };

  temp *w_args = (temp*) args;
  client_args *pcb = w_args->pcb;
  pthread_mutex_t *mutex = w_args->mutex;
  pthread_cond_t *podeCons = w_args->podeCons;
  pthread_cond_t *podeProd = w_args->podeProd;
  int *consptr = w_args->consptr;
  int *count = w_args->count;

  S++;

  client_args c_args;
  while (1) {
    pthread_mutex_lock(mutex);
    while (*count == 0) pthread_cond_wait(podeCons,mutex);
    c_args = pcb[*consptr];
    (*consptr)++;
    if (*consptr == MAX_SESSION_COUNT) {
      *consptr = 0;
    }
    (*count)--;
    pthread_cond_signal(podeProd);
    pthread_mutex_unlock(mutex);

    char *req_pipe_path = c_args.req_pipe_path;
    char *resp_pipe_path = c_args.resp_pipe_path;

    int req_pipe_fd = safe_open(req_pipe_path, O_RDONLY);
    int resp_pipe_fd = safe_open(resp_pipe_path, O_WRONLY);

    fprintf(stderr, "[INFO] Worker thread started\n");

    S++;
    safe_write(resp_pipe_fd, &S, sizeof(int));

    char code;

    do {
        safe_read(req_pipe_fd, &code, sizeof(char));
        int session;
        safe_read(req_pipe_fd, &session, sizeof(int));

        switch (code) {
            case QUIT_CODE:
              close(req_pipe_fd);
              close(resp_pipe_fd);
              break;
            case CREATE_CODE:
              unsigned int event_id_c;
              size_t num_rows;
              size_t num_cols;
              
              safe_read(req_pipe_fd, &event_id_c, sizeof(unsigned int));
              safe_read(req_pipe_fd, &num_rows, sizeof(size_t));
              safe_read(req_pipe_fd, &num_cols, sizeof(size_t));

              int res = ems_create(event_id_c, num_rows, num_cols);
              safe_write(resp_pipe_fd, &res, sizeof(int));
              break;
            case SHOW_CODE:
              unsigned int event_id_s;
              safe_read(req_pipe_fd, &event_id_s, sizeof(unsigned int));
              
              int res3 = ems_show(resp_pipe_fd, event_id_s);
              if(res3 == 1) {
                fprintf(stderr, "[ERR] Failed to show event\n");
                safe_write(resp_pipe_fd, &res3, sizeof(int));
              }
              break;
            case LIST_CODE:
              int res4 = ems_list_events(resp_pipe_fd);
              
              if(res4 == 1) {
                fprintf(stderr, "[ERR] Failed to list events\n");
                safe_write(resp_pipe_fd, &res4, sizeof(int));
              }
              break;
            case RESERVE_CODE:
              fprintf(stderr, "[INFO] Reserving seats\n");
              unsigned int event_id_r;
              size_t num_seats;
              
              safe_read(req_pipe_fd, &event_id_r, sizeof(unsigned int));
              safe_read(req_pipe_fd, &num_seats, sizeof(size_t));

              size_t xs[num_seats];
              size_t ys[num_seats];

              safe_read(req_pipe_fd, &xs, sizeof(size_t[num_seats]));
              safe_read(req_pipe_fd, &ys, sizeof(size_t[num_seats]));

              int res2 = ems_reserve(event_id_r, num_seats, xs, ys);
              safe_write(resp_pipe_fd, &res2, sizeof(int));
              break;
        }
    } while (code != QUIT_CODE);

    fprintf(stderr, "[Info] Worker thread ended\n");
    // close(req_pipe_fd);
    // close(resp_pipe_fd);
  }

  return NULL;
}

void *main_thread_func(void *pathname) {
  if (signal(SIGUSR1, sig_handler) == SIG_ERR) {
    fprintf(stderr, "TODO\n");
    exit(EXIT_FAILURE);
  }

  char *server_pathname = (char*) pathname;

  client_args pcb[MAX_SESSION_COUNT];
  memset(pcb, 0, sizeof(pcb));
  int prodptr = 0, consptr = 0, count = 0;
  pthread_mutex_t mutex;
  if (pthread_mutex_init(&mutex, NULL) != 0) {
    fprintf(stderr, "Failed do initialize mutex\n");
    return NULL;
  }
  pthread_cond_t podeProd, podeCons;
  if (pthread_cond_init(&podeProd, NULL) != 0) {
    fprintf(stderr, "Failed do initialize conditional variable\n");
    return NULL;
  }
  if (pthread_cond_init(&podeCons, NULL) != 0) {
    fprintf(stderr, "Failed do initialize conditional variable\n");
    return NULL;
  }

  open_pipe(server_pathname, SERVER_PIPE_MODE);
  server_fd = safe_open(server_pathname, O_RDONLY);

  pthread_t worker_threads[MAX_SESSION_COUNT];

  for (int i = 0; i < MAX_SESSION_COUNT; i++) {
    temp *arg = (temp*) safe_malloc(sizeof(temp));
    arg->pcb = pcb;
    arg->consptr = &consptr;
    arg->prodptr = &prodptr;
    arg->count = &count;
    arg->mutex = &mutex;
    arg->podeCons = &podeCons;
    arg->podeProd = &podeProd;
    if (pthread_create(&worker_threads[i], NULL, worker_thread_func, arg) != 0) {
      fprintf(stderr, "Failed to create worker thread\n"); // TODO: verificar se é o comportamento esperado
      return NULL;
    }
  }

  while (1) {
    client_args item = produz(server_fd);
    pthread_mutex_lock(&mutex);
    while (count == MAX_SESSION_COUNT) {
      if (cond) {
        // TODO
        ems_list_and_show();
        cond = 0;
      }
    pthread_cond_wait(&podeProd, &mutex);
    }
      pcb[prodptr++] = item;
      if (prodptr == MAX_SESSION_COUNT) {
        prodptr = 0;
      }
      count++;
      pthread_cond_signal(&podeCons);
      pthread_mutex_unlock(&mutex);
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

  // while (1) {
    // TODO: Read from pipe
    // TODO: Write new client to the producer-consumer buffer
  // }

  //TODO: Close Server

  return ems_terminate();
}
