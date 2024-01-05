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

void *safe_malloc(size_t size) {
  void *ptr = malloc(size);
  if (ptr == NULL) {
    fprintf(stderr, "[ERR]: Failed to allocate memory\n");
    exit(EXIT_FAILURE);
  }
  return ptr;
}

void safe_read(int fd, void *buf, size_t size) {
  ssize_t bytes_read;

  do {
    bytes_read = read(fd, buf, size);
    if (bytes_read < 1) {
      if (bytes_read == 0) {
        fprintf(stderr, "[ERR]: Unexpected end of file\n");
      } else if (errno == EINTR) {
        continue;
      } else {
        fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
      }
    }
  } while (bytes_read == -1);
}

int safe_write(int fd, const void *buf, size_t size) {
  do {
    ssize_t bytes_written = write(fd, buf, size);
    if (bytes_written == -1) {
      if (errno == EPIPE) {
        return 2;
      } else {
        fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
        return 1;
      }
    }

    size -= (size_t)bytes_written;
  } while (size > 0);

  return 0;
}

int safe_open(const char *pathname, int flags) {
  int fd = open(pathname, flags);

  if (fd == -1) {
    fprintf(stderr, "[Err]: open failed: %s\n", strerror(errno));
  }

  return fd;
}

void safe_close(int fd) {
  if (close(fd) == -1) {
    fprintf(stderr, "[ERR]: close failed: %s\n", strerror(errno));
  }
}

void open_pipe(const char *pathname, mode_t mode) {
  safe_unlink(pathname);

  if (mkfifo(pathname, mode) != 0) {
    fprintf(stderr, "[ERR]: mkfifo failed: %s\n", strerror(errno));
  }

  fprintf(stdout, "[INFO]: Pipe %s created\n", pathname);
}

void safe_unlink(const char *pathname) {
  if (unlink(pathname) != 0 && errno != ENOENT) {
    fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", pathname, strerror(errno));
  }
}
