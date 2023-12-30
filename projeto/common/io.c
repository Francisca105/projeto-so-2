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

void *safe_malloc(size_t size) {
  void *ptr = malloc(size);
  if (ptr == NULL) {
    fprintf(stderr, "Failed to allocate memory\n");
    exit(1);
  }
  return ptr;
}

void safe_write(int fd, void *buffer, size_t size) {
  if(write(fd, buffer, size) == -1) {
    fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
}

void safe_read(int fd, void *buffer, size_t size) {
  ssize_t r = read(fd, buffer, size);

  if(r == -1) {
    fprintf(stderr, "[Err]: read failed: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  } else if (r == 0) {
    fprintf(stderr, "[Err]: unexpected end of file\n");
    exit(EXIT_FAILURE);
  }
}

int safe_open(const char *pathname, int flags) {
  int fd = open(pathname, flags);

  if(fd == -1) {
    fprintf(stderr, "[Err]: open failed: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  return fd;
}

void open_pipe(const char *pathname, mode_t mode) {
  if (unlink(pathname) != 0 && errno != ENOENT) {
    fprintf(stderr, "unlink(%s) failed: %s\n", pathname, strerror(errno));
    exit(EXIT_FAILURE);
  }

  if (mkfifo(pathname, mode) != 0) {
    fprintf(stderr, "mkfifo failed: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
}