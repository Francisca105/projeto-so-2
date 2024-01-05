#include "threads.h"

#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "common/constants.h"
#include "common/io.h"
#include "operations.h"

#define WRITE_TO_PIPE(fd, data, size) if (safe_write(fd, data, size) == 2) goto end

int sig_flag = 0;

void sig_handler(int sig) {
  if (sig == SIGUSR1) {
    if (signal(SIGUSR1, sig_handler) == SIG_ERR) {
      sig_flag = 2;      
    }
    sig_flag = 1;
  }
}

client_args produce(int fd) {
  client_args ret;
  
  char code;
  safe_read_s(fd, &code, sizeof(char), &sig_flag, ems_list_and_show);
  
  char req_pipe_path[PIPE_NAME_SIZE];
  char resp_pipe_path[PIPE_NAME_SIZE];

  safe_read_s(fd, req_pipe_path, PIPE_NAME_SIZE, &sig_flag, ems_list_and_show);
  safe_read_s(fd, resp_pipe_path, PIPE_NAME_SIZE, &sig_flag, ems_list_and_show);

  strncpy(ret.req_pipe_path, req_pipe_path, PIPE_NAME_SIZE);
  strncpy(ret.resp_pipe_path, resp_pipe_path, PIPE_NAME_SIZE);

  if (sig_flag) {
    if (sig_flag == 2) fprintf(stderr, "[ERR]: Failed to change how SIGUSR1 is handled\n");
    ems_list_and_show();
    sig_flag = 0;
  } 

  return ret;
}

void *worker_thread_func(void *args) {
  // Ignores the SIGUSR1 signal
  sigset_t set;
  sigemptyset(&set);
  sigaddset(&set, SIGUSR1);
  if (pthread_sigmask(SIG_BLOCK, &set, NULL) != 0) {
    fprintf(stderr, "[ERR]: Failed masking SIGUSR1\n");
  };

  worker_args *w_args = (worker_args*) args;
  client_args *prodConsBuf = w_args->prodConsBuf;
  pthread_mutex_t *mutex = w_args->mutex;
  pthread_cond_t *podeCons = w_args->podeCons;
  pthread_cond_t *podeProd = w_args->podeProd;
  int *consptr = w_args->consptr;
  int *count = w_args->count;
  int session_id = w_args->session_id;

  client_args c_args;

  while (1) {
    // Fetches an entry of the producer-consumer buffer (if its empty, waits)
    pthread_mutex_lock(mutex);
    while (*count == 0) pthread_cond_wait(podeCons,mutex);
    c_args = prodConsBuf[*consptr];
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

    fprintf(stdout, "[INFO]: Client started a new session: %d\n", session_id);

    WRITE_TO_PIPE(resp_pipe_fd, &session_id, sizeof(int));

    char code;

    do {
        safe_read(req_pipe_fd, &code, sizeof(char));
        int session;
        safe_read(req_pipe_fd, &session, sizeof(int));

        switch (code) {
            case QUIT_CODE: {
              break;
            }
            case CREATE_CODE: {
              unsigned int event_id_c;
              size_t num_rows;
              size_t num_cols;
              
              safe_read(req_pipe_fd, &event_id_c, sizeof(unsigned int));
              safe_read(req_pipe_fd, &num_rows, sizeof(size_t));
              safe_read(req_pipe_fd, &num_cols, sizeof(size_t));

              int res = ems_create(event_id_c, num_rows, num_cols);
              WRITE_TO_PIPE(resp_pipe_fd, &res, sizeof(int));
              break;
            }
            case SHOW_CODE: {
              unsigned int event_id_s;
              safe_read(req_pipe_fd, &event_id_s, sizeof(unsigned int));
              
              int res = ems_show(resp_pipe_fd, event_id_s);
              if(res == 1) {
                fprintf(stderr, "Failed to show event\n");
                WRITE_TO_PIPE(resp_pipe_fd, &res, sizeof(int));
              } else if (res == 2) {
                goto end;
              }
              break;
            }
            case LIST_CODE: {
              int res = ems_list_events(resp_pipe_fd);
              
              if(res == 1) {
                fprintf(stderr, "Failed to list events\n");
                WRITE_TO_PIPE(resp_pipe_fd, &res, sizeof(int));
              } else if (res == 2) {
                goto end;
              }
              break;
            }
            case RESERVE_CODE: {
              unsigned int event_id_r;
              size_t num_seats;
              
              safe_read(req_pipe_fd, &event_id_r, sizeof(unsigned int));
              safe_read(req_pipe_fd, &num_seats, sizeof(size_t));

              size_t xs[num_seats];
              size_t ys[num_seats];

              safe_read(req_pipe_fd, &xs, sizeof(size_t[num_seats]));
              safe_read(req_pipe_fd, &ys, sizeof(size_t[num_seats]));

              int res = ems_reserve(event_id_r, num_seats, xs, ys);
              WRITE_TO_PIPE(resp_pipe_fd, &res, sizeof(int));
              break;
            }
        }
    } while (code != QUIT_CODE);

    end:
      fprintf(stdout, "[INFO]: Client ended session: %d\n", session_id);
      close(req_pipe_fd);
      close(resp_pipe_fd);
  }
}

void *main_thread_func(void *arg) {
  if (signal(SIGUSR1, sig_handler) == SIG_ERR) {
    fprintf(stderr, "[ERR]: Failed to change how SIGUSR1 is handled\n");
  }

  sigset_t set;
  sigemptyset(&set);
  sigaddset(&set, SIGPIPE);
  if (pthread_sigmask(SIG_BLOCK, &set, NULL) != 0) {
    fprintf(stderr, "[ERR]: Failed masking SIGPIPE\n");
  };

  char *server_pathname = (char*) arg;

  // producer-consumer buffer
  client_args prodConsBuf[MAX_SESSION_COUNT];
  memset(prodConsBuf, 0, sizeof(prodConsBuf));

  int prodptr = 0, consptr = 0, count = 0;

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
}
