#include "api.h"
#include "common/io.h"
#include "common/constants.h"

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

int server_fd;
int req_pipe_fd;
int resp_pipe_fd;

int ems_setup(char const* req_pipe_path, char const* resp_pipe_path, char const* server_pipe_path) {
  //TODO: create pipes and connect to the server
  open_pipe(req_pipe_path, REQ_PIPE_MODE);
  open_pipe(resp_pipe_path, RESP_PIPE_MODE);

  // Server pipe
  server_fd = safe_open(server_pipe_path, O_WRONLY);
  
  char code = '1';
  safe_write(server_fd, &code, sizeof(code));
  safe_write(server_fd, req_pipe_path, MAX_PIPE_NAME);
  safe_write(server_fd, resp_pipe_path, MAX_PIPE_NAME);

  close(server_fd);
  // 

  req_pipe_fd = safe_open(req_pipe_path, O_WRONLY);
  resp_pipe_fd = safe_open(resp_pipe_path, O_RDONLY);
  
  int session;
  // safe_read(resp_pipe_fd, &session, sizeof(int));
  fprintf(stderr, "[Info] New session with Id: %d\n", session);

  return 1;
}

int ems_quit(void) { 
  //TODO: close pipes
  char code = '2';
  safe_write(req_pipe_fd, &code, sizeof(code));

  close(req_pipe_fd);
  close(resp_pipe_fd);

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
