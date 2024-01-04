#include "api.h"

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#include "common/io.h"
#include "common/constants.h"

int req_pipe_fd;
int resp_pipe_fd;
char const* req_pipe_p;
char const* resp_pipe_p;
int session_id;

// TODO: ver onde isto pode falhar e retornar 0 aí
int ems_setup(char const* req_pipe_path, char const* resp_pipe_path, char const* server_pipe_path) {
  req_pipe_p = req_pipe_path;
  resp_pipe_p = resp_pipe_path;

  open_pipe(req_pipe_path, PIPE_PERMS);
  open_pipe(resp_pipe_path, PIPE_PERMS);

  // Server pipe
  int server_fd = safe_open(server_pipe_path, O_WRONLY);

  char msg[1+2*PIPE_NAME_SIZE];
  msg[0] = '1';
  strncpy(msg + 1, req_pipe_path, PIPE_NAME_SIZE);
  strncpy(msg + 1 + PIPE_NAME_SIZE, resp_pipe_path, PIPE_NAME_SIZE);

  safe_write(server_fd, msg, 1 + 2*PIPE_NAME_SIZE);

  safe_close(server_fd);

  req_pipe_fd = safe_open(req_pipe_path, O_WRONLY);
  resp_pipe_fd = safe_open(resp_pipe_path, O_RDONLY);
  
  safe_read(resp_pipe_fd, &session_id, sizeof(int));
  fprintf(stdout, "[INFO]: New session with id %d\n", session_id);

  return 0;
}

int ems_quit(void) { 
  char code = '2';
  safe_write(req_pipe_fd, &code, sizeof(code));
  safe_write(req_pipe_fd, &session_id, sizeof(int));

  safe_close(req_pipe_fd);
  safe_close(resp_pipe_fd);

  safe_unlink(req_pipe_p);
  safe_unlink(resp_pipe_p);
  printf("[INFO]: Closed session_id %d pipes\n", session_id);

  return 0;
}

int ems_create(unsigned int event_id, size_t num_rows, size_t num_cols) {
  char code = '3';
  safe_write(req_pipe_fd, &code, sizeof(code));
  safe_write(req_pipe_fd, &session_id, sizeof(int));
  safe_write(req_pipe_fd, &event_id, sizeof(unsigned int));
  safe_write(req_pipe_fd, &num_rows, sizeof(size_t));
  safe_write(req_pipe_fd, &num_cols, sizeof(size_t));

  int res;
  safe_read(resp_pipe_fd, &res, sizeof(int));

  return res;
}

int ems_reserve(unsigned int event_id, size_t num_seats, size_t* xs, size_t* ys) {
  char code = '4';
  safe_write(req_pipe_fd, &code, sizeof(code));
  safe_write(req_pipe_fd, &session_id, sizeof(int));
  safe_write(req_pipe_fd, &event_id, sizeof(unsigned int));
  safe_write(req_pipe_fd, &num_seats, sizeof(size_t));
  safe_write(req_pipe_fd, xs, sizeof(size_t[num_seats]));
  safe_write(req_pipe_fd, ys, sizeof(size_t[num_seats]));

  int res;
  safe_read(resp_pipe_fd, &res, sizeof(int));

  return res;
}

int ems_show(int out_fd, unsigned int event_id) {
  char code = '5';
  safe_write(req_pipe_fd, &code, sizeof(code));
  safe_write(req_pipe_fd, &session_id, sizeof(int));
  safe_write(req_pipe_fd, &event_id, sizeof(unsigned int));

  int res;
  safe_read(resp_pipe_fd, &res, sizeof(int));

  if(res == 1) return res;

  size_t num_rows;
  safe_read(resp_pipe_fd, &num_rows, sizeof(size_t));
  size_t num_cols;
  safe_read(resp_pipe_fd, &num_cols, sizeof(size_t));

  unsigned int seats[num_rows*num_cols];
  safe_read(resp_pipe_fd, &seats, sizeof(unsigned int[num_rows*num_cols]));

  for(size_t i = 0; i < num_rows*num_cols; i++) {
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
  safe_write(req_pipe_fd, &session_id, sizeof(int));

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
