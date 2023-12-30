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

int S = 0;

typedef struct {
  char req_pipe_path[MAX_PIPE_NAME];
  char resp_pipe_path[MAX_PIPE_NAME];
} client_args;

void *worker_thread_func(void *args) {
  client_args *c_args = (client_args*) args;

  char *req_pipe_path = c_args->req_pipe_path;
  char *resp_pipe_path = c_args->resp_pipe_path;
  int req_pipe_fd = safe_open(req_pipe_path, O_RDONLY);
  int resp_pipe_fd = safe_open(resp_pipe_path, O_WRONLY);

  fprintf(stderr, "Worker thread started\n");

  S++;
  safe_write(resp_pipe_fd, &S, sizeof(int));

  char code;

  do {
      safe_read(req_pipe_fd, &code, sizeof(char));

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
            // TODO: Implement
            // unsigned int event_id_s;
            // safe_read(req_pipe_fd, &event_id_s, sizeof(unsigned int));

            // int res3 = ems_show(resp_pipe_fd, event_id_s);
            // safe_write(resp_pipe_fd, &res3, sizeof(int));
          break;
          // This need to be the last case to work properly
          case RESERVE_CODE:
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


  // while(1) {
  //   if(first) {
  //     first = 0;
  //     int session = 0;
  //     // safe_write(resp_pipe_fd, &session, sizeof(int)+1); // Problematico TODO
  //   }
  //   printf("Waiting for request...\n");
  //   char code;
  //   safe_read(req_pipe_fd, &code, sizeof(char));
  //   printf("Code: %c\n", code);

  //   switch (code){
  //   // case QUIT_CODE:
      
  //   //   break;
  //   default:break;
  //   }
  // }

  fprintf(stderr, "Worker thread ended\n");
  close(req_pipe_fd);
  close(resp_pipe_fd);
  free(args);

  return NULL;
}

void *main_thread_func(void *pathname) {
  char *server_pathname = (char*) pathname;

  open_pipe(server_pathname, SERVER_PIPE_MODE);

  while (1) {
    printf("Waiting for client...\n");
    int server_fd = safe_open(server_pathname, O_RDONLY);

    char code;
    safe_read(server_fd, &code, sizeof(char));

    // Unecessary to check the code due to the message format being always correct
    char req_pipe_path[MAX_PIPE_NAME];
    char resp_pipe_path[MAX_PIPE_NAME];
    safe_read(server_fd, req_pipe_path, MAX_PIPE_NAME);
    safe_read(server_fd, resp_pipe_path, MAX_PIPE_NAME);
    fprintf(stderr, "Request pipe: %s\n", req_pipe_path);
    fprintf(stderr, "Response pipe: %s\n", resp_pipe_path);

    client_args *args = (client_args*) malloc(sizeof(client_args));
    strncpy(args->req_pipe_path, req_pipe_path, MAX_PIPE_NAME);
    strncpy(args->resp_pipe_path, resp_pipe_path, MAX_PIPE_NAME);

    // TODO: Multi-threaded server
    pthread_t worker_thread;
    if (pthread_create(&worker_thread, NULL, worker_thread_func, args) != 0) {
      fprintf(stderr, "Failed to create worker thread\n");
      exit(EXIT_FAILURE);
    }
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
