#ifndef HEADER_FILE_API_H
#define HEADER_FILE_API_H

#include <stdint.h>
#include <stdlib.h>

typedef struct file_s file_handle;

file_handle *file_open(const char *filename, const char *mode);

size_t file_read(void *buffer, size_t element_size, size_t element_count, file_handle *stream);

char *file_gets(char *buffer, int32_t max_count, file_handle *stream);

int32_t file_getc(file_handle *stream);

int64_t file_seek(file_handle *stream, int64_t offset, int32_t origin);

size_t file_write(const void *buffer, size_t size, size_t count, file_handle *stream);

int32_t file_putc(int32_t character, file_handle *stream);

int32_t file_printf(file_handle *stream, const char *format, const char *value);

uint64_t file_tell(file_handle *stream);

int32_t file_close(file_handle *stream);

#endif
