#include "constants.h"
#include "io.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

int parse_uint(int fd, unsigned int *value, char *next) {
  char buf[16];

  int i = 0;
  while (1) {
    ssize_t read_bytes = read(fd, buf + i, 1);
    if (read_bytes == -1) {
      return 1;
    } else if (read_bytes == 0) {
      *next = '\0';
      break;
    }

    *next = buf[i];

    if (buf[i] > '9' || buf[i] < '0') {
      buf[i] = '\0';
      break;
    }

    i++;
  }

  unsigned long ul = strtoul(buf, NULL, 10);

  if (ul > UINT_MAX) {
    return 1;
  }

  *value = (unsigned int)ul;

  return 0;
}

int print_uint(int fd, unsigned int value) {
  char buffer[16];
  size_t i = 16;

  for (; value > 0; value /= 10) {
    buffer[--i] = '0' + (char)(value % 10);
  }

  if (i == 16) {
    buffer[--i] = '0';
  }

  while (i < 16) {
    ssize_t written = write(fd, buffer + i, 16 - i);
    if (written == -1) {
      return 1;
    }

    i += (size_t)written;
  }

  return 0;
}

int print_str(int fd, const char *str) {
  size_t len = strlen(str);
  while (len > 0) {
    ssize_t written = write(fd, str, len);
    if (written == -1) {
      return 1;
    }

    str += (size_t)written;
    len -= (size_t)written;
  }

  return 0;
}

// Begin of the changes 

// Correia work: https://github.com/ist199211-ist199341/tecnicofs-so/blob/master/common/common.c
ssize_t try_read(int fd, void *buf, size_t count) {
    ssize_t bytes_read;
    do {
        bytes_read = read(fd, buf, count);
    } while (bytes_read < 0 && errno == EINTR);
    return bytes_read;
}

ssize_t try_write(int fd, const void *buf, size_t count) {
    ssize_t bytes_written;
    do {
        bytes_written = write(fd, buf, count);
    } while (bytes_written < 0 && errno == EINTR);
    return bytes_written;
}

void read_pipe_line(int fd, char *buffer, size_t buffer_size) {
  size_t i = 0;
  while (i < buffer_size) {
    ssize_t read_bytes = read(fd, buffer + i, 1);
    if (read_bytes == -1) {
      fprintf(stderr, "[ERR]: read failed\n");
      exit(EXIT_FAILURE);
    } else if (read_bytes == 0) {
      buffer[i] = '\0';
      break;
    }

    if (buffer[i] == '\n') {
      buffer[i] = '\0';
      break;
    }

    i++;
  }
}

void send_char_msg(int tx, char const *str) {
  size_t len = strlen(str);
  size_t written = 0;

  while (written < len) {
    ssize_t ret = write(tx, str + written, len - written);
    if (ret < 0) {
      fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
    } else {
      written += ret;
    }
  }
}

// char * setup_msg(char *request_pipe, char *response_pipe) {
//   char *buffer;

//   memcpy(buffer, SETUP_CODE, sizeof(char)); 
//   memcpy(buffer + sizeof(char), request_pipe, MAX_PIPE_NAME);
//   memcpy(buffer + sizeof(char) + MAX_PIPE_NAME, response_pipe, MAX_PIPE_NAME);

//   return buffer;
// }