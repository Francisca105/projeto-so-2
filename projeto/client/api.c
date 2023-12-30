#include "api.h"
#include "common/io.h"
#include "common/constants.h"

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

int ems_setup(char const* req_pipe_path, char const* resp_pipe_path, char const* server_pipe_path) {
  //TODO: create pipes and connect to the server
  open_pipe(req_pipe_path, REQ_PIPE_MODE);
  open_pipe(resp_pipe_path, RESP_PIPE_MODE);

  int server = open(server_pipe_path, O_WRONLY);
  
  if (server == -1) {
    perror("Failed to open server pipe (via Client)\n");
    exit(EXIT_FAILURE);
  }
  
  char code = SETUP_CODE;
  safe_write(server, &code, sizeof(code));
  safe_write(server, req_pipe_path, MAX_PIPE_NAME);
  safe_write(server, resp_pipe_path, MAX_PIPE_NAME);
  
  return 1;
}

int ems_quit(void) { 
  //TODO: close pipes

  return 1;
}

int ems_create(unsigned int event_id, size_t num_rows, size_t num_cols) {
  //TODO: send create request to the server (through the request pipe) and wait for the response (through the response pipe)
  return 1;
}

int ems_reserve(unsigned int event_id, size_t num_seats, size_t* xs, size_t* ys) {
  //TODO: send reserve request to the server (through the request pipe) and wait for the response (through the response pipe)
  return 1;
}

int ems_show(int out_fd, unsigned int event_id) {
  //TODO: send show request to the server (through the request pipe) and wait for the response (through the response pipe)
  return 1;
}

int ems_list_events(int out_fd) {
  //TODO: send list request to the server (through the request pipe) and wait for the response (through the response pipe)
  return 1;
}
