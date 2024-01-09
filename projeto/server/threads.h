#ifndef THREADS_H
#define THREADS_H

#include <pthread.h>

#include "common/constants.h"

typedef struct {
  char req_pipe_path[PIPE_NAME_SIZE];
  char resp_pipe_path[PIPE_NAME_SIZE];
} client_args;

typedef struct {
  client_args* prodConsBuf;
  int *prodptr, *consptr, *count, session_id;
  pthread_mutex_t *mutex;
  pthread_cond_t *podeProd, *podeCons;
} worker_args;

/// Handles the SIGUSR1 signal by setting a flag to true.
/// @param sig signal
void sig_handler(int sig);

/// Produces an entry to put in the producer-consumer buffer.
/// Reads the client pipe names from the server pipe.
/// @param fd file descriptor of the server pipe
/// @return the entry to put in the producer-consumer buffer.
client_args produce(int fd);

/// Worker thread function.
/// @param args pointer to the thread arguments
/// @return it should never return
void *worker_thread_func(void *args);

#endif