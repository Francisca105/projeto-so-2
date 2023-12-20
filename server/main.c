#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>

#include "common/constants.h"
#include "common/io.h"
#include "operations.h"

int S = 1; // TODO: session counter

int main(int argc, char* argv[]) {
  pthread_t main_thread;

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

  // Garanties that the pipe is not associated with any other process
  unlink(argv[1]);
  
  // Creates the pipe
  if (mkfifo(argv[1], 0777)) {
    fprintf(stderr, "Failed to create pipe\n");
    return 1;
  }

  // Creates safe thread
  if (pthread_create(main_thread,NULL, NULL,NULL) == -1) {
    fprintf(stderr, "Failed to create main thread\n");
    return 1;
  }

  while (1) {
    //TODO: Read from pipe
    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
      fprintf(stderr, "Failed to open pipe\n");
      return 1;
    }
    
    //TODO: Write new client to the producer-consumer buffer
  }

  //TODO: Close Server

  ems_terminate();
}