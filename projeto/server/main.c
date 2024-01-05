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

  char server_pathname[PIPE_PERMS];
  strncpy(server_pathname, argv[1], PIPE_NAME_SIZE);
  
  pthread_t server;
  if (pthread_create(&server, NULL, main_thread_func, server_pathname) != 0) {
    fprintf(stderr, "Failed to create main thread\n");
    return 1;
  } 
  
  // Server should not end
  void *ret;
  if (pthread_join(server, &ret) != 0) {
    fprintf(stderr, "Failed to join server\n");
    return 1;
  } else if (*(int*) ret != 0) {
    fprintf(stdin, "Server exited prematurely\n");
  }

  return ems_terminate();
}
