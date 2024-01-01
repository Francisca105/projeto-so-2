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
char *req_pipe_p;
char *resp_pipe_p;
int session;

int ems_setup(char const* req_pipe_path, char const* resp_pipe_path, char const* server_pipe_path) {
  //TODO: create pipes and connect to the server
  req_pipe_p = req_pipe_path;
  resp_pipe_p = resp_pipe_path;

  open_pipe(req_pipe_path, REQ_PIPE_MODE);
  open_pipe(resp_pipe_path, RESP_PIPE_MODE);

  // Server pipe
  server_fd = safe_open(server_pipe_path, O_WRONLY);
  
  char code = '1';
  safe_write(server_fd, &code, sizeof(code));
  safe_write(server_fd, req_pipe_path, MAX_PIPE_NAME);
  safe_write(server_fd, resp_pipe_path, MAX_PIPE_NAME);

  close(server_fd);

  req_pipe_fd = safe_open(req_pipe_path, O_WRONLY);
  resp_pipe_fd = safe_open(resp_pipe_path, O_RDONLY);
  
  safe_read(resp_pipe_fd, &session, sizeof(int));
  fprintf(stderr, "[Info] New session with Id: %d\n", session);

  return 0;
}

int ems_quit(void) { 
  //TODO: close pipes
  char code = '2';
  safe_write(req_pipe_fd, &code, sizeof(code));
  safe_write(req_pipe_fd, &session, sizeof(int));

  close(req_pipe_fd);
  close(resp_pipe_fd);

  close_pipe(req_pipe_p);
  close_pipe(resp_pipe_p);
  printf("[Info] Closed session %d pipes\n", session);

  return 0;
}

int ems_create(unsigned int event_id, size_t num_rows, size_t num_cols) {
  //TODO: send create request to the server (through the request pipe) and wait for the response (through the response pipe)
  char code = '3';
  safe_write(req_pipe_fd, &code, sizeof(code));
  safe_write(req_pipe_fd, &session, sizeof(int));
  safe_write(req_pipe_fd, &event_id, sizeof(unsigned int));
  safe_write(req_pipe_fd, &num_rows, sizeof(size_t));
  safe_write(req_pipe_fd, &num_cols, sizeof(size_t));

  int res;
  safe_read(resp_pipe_fd, &res, sizeof(int));
  return res;
}

int ems_reserve(unsigned int event_id, size_t num_seats, size_t* xs, size_t* ys) {
  //TODO: send reserve request to the server (through the request pipe) and wait for the response (through the response pipe)
  char code = '4';
  safe_write(req_pipe_fd, &code, sizeof(code));
  safe_write(req_pipe_fd, &session, sizeof(int));
  safe_write(req_pipe_fd, &event_id, sizeof(unsigned int));
  safe_write(req_pipe_fd, &num_seats, sizeof(size_t));
  safe_write(req_pipe_fd, xs, sizeof(size_t[num_seats]));
  safe_write(req_pipe_fd, ys, sizeof(size_t[num_seats]));

  int res;
  safe_read(resp_pipe_fd, &res, sizeof(int));
  return res;
}

int ems_show(int out_fd, unsigned int event_id) {
  //TODO: send show request to the server (through the request pipe) and wait for the response (through the response pipe)
  char code = '5';
  safe_write(req_pipe_fd, &code, sizeof(code));
  safe_write(req_pipe_fd, &session, sizeof(int));

  safe_write(req_pipe_fd, &event_id, sizeof(unsigned int));

  int res;
  safe_read(resp_pipe_fd, &res, sizeof(int));

  if(res == 1) return res;

  size_t num_rows;
  size_t num_cols;
  safe_read(resp_pipe_fd, &num_rows, sizeof(size_t));
  safe_read(resp_pipe_fd, &num_cols, sizeof(size_t));

  unsigned int seats[num_rows*num_cols];
  safe_read(resp_pipe_fd, &seats, sizeof(unsigned int[num_rows*num_cols]));

  // safe_write(out_fd, &res, sizeof(int));

  char newline = '\n';
  char space = ' ';
  for(size_t i = 0; i < num_rows*num_cols; i++) {
    unsigned int seat = seats[i];
    print_uint(out_fd, seats[i]);

    if((i+1) % num_cols == 0) {
        print_str(out_fd, "\n");
    } else {
        print_str(out_fd, " ");
    }
  }


  return 0;
}

int ems_list_events(int out_fd) {
  char code = '6';
  safe_write(req_pipe_fd, &code, sizeof(code));
  safe_write(req_pipe_fd, &session, sizeof(int));

  int res;
  safe_read(resp_pipe_fd, &res, sizeof(int));

  if(res == 1) return res;

  size_t num_events;
  safe_read(resp_pipe_fd, &num_events, sizeof(size_t));
  unsigned int event_ids[num_events];
  if(num_events == 0) {
    print_str(out_fd, "No events\n");
    return 0;
  }
  safe_read(resp_pipe_fd, &event_ids, sizeof(unsigned int[num_events]));

  for(size_t i = 0; i < num_events; i++) {
    print_str(out_fd, "Event: ");
    print_uint(out_fd, event_ids[i]);
    print_str(out_fd, "\n");
  }
  return 0;
}
