#ifndef COMMON_IO_H
#define COMMON_IO_H

#include <sys/types.h>

/// Parses an unsigned integer from the given file descriptor.
/// @param fd The file descriptor to read from.
/// @param value Pointer to the variable to store the value in.
/// @param next Pointer to the variable to store the next character in.
/// @return 0 if the integer was read successfully, 1 otherwise.
int parse_uint(int fd, unsigned int *value, char *next);

/// Prints an unsigned integer to the given file descriptor.
/// @param fd The file descriptor to write to.
/// @param value The value to write.
/// @return 0 if the integer was written successfully, 1 otherwise.
int print_uint(int fd, unsigned int value);

/// Writes a string to the given file descriptor.
/// @param fd The file descriptor to write to.
/// @param str The string to write.
/// @return 0 if the string was written successfully, 1 otherwise.
int print_str(int fd, const char *str);

/// Safely reads something from the given file descriptor.
/// @param fd The file descriptor to read from.
/// @param buf Pointer to the variable to store the string in.
/// @param size The maximum number of bytes to read.
void safe_read(int fd, void *buf, size_t size);

/// Safely writes something to the given file descriptor.
/// @param fd The file descriptor to write to.
/// @param buf Pointer to the variable to read the string from.
/// @param size The number of bytes to write.
/// @return 0 if the write was sucessfull, 1 if failed and 2 if EPIPE occured
int safe_write(int fd, const void *buf, size_t size);

/// Creates a malloc with error checking.
/// @param size 
/// @return the pointer of the malloc
void *safe_malloc(size_t size);

/// Safely opens a file.
/// @param pathname
/// @param flags
/// @return fd
int safe_open(const char *pathname, int flags);

/// Safely closes a file.
/// @param fd 
void safe_close(int fd);

/// Safely opens a pipe.
/// @param pathname
/// @param mode
void open_pipe(const char *pathname, mode_t mode);

/// Safely unlink a file.
/// @param pathname
void safe_unlink(const char *pathname);

#endif  // COMMON_IO_H