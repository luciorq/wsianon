#include "file-api.h"

#include <inttypes.h>
#include <stdio.h>

struct file_s {
    FILE *fp;
};

file_handle *file_open(const char *filename, const char *mode) {
    file_handle *stream = NULL;

    FILE *fp = fopen(filename, mode);

    if (fp != NULL) {
        stream = (file_handle *)malloc(sizeof(file_handle));
        stream->fp = fp;
    }

    return stream;
}

size_t file_read(void *buffer, size_t element_size, size_t element_count, file_handle *stream) {
    return fread(buffer, element_size, element_count, stream->fp);
}

char *file_gets(char *buffer, int32_t max_count, file_handle *stream) { return fgets(buffer, max_count, stream->fp); }

int32_t file_getc(file_handle *stream) { return fgetc(stream->fp); }

int64_t file_seek(file_handle *stream, int64_t offset, int32_t origin) {
#ifdef __linux__
    return fseeko(stream->fp, offset, origin);
#elif defined(__APPLE__) && defined(__MACH__)
    return fseeko(stream->fp, offset, origin);
#elif _WIN32
    return _fseeki64(stream->fp, offset, origin);
#elif _WIN64
    return _fseeki64(stream->fp, offset, origin);
#else
    return -1;
#endif
}

size_t file_write(const void *buffer, size_t size, size_t count, file_handle *stream) {
    return fwrite(buffer, size, count, stream->fp);
}

int32_t file_putc(int32_t character, file_handle *stream) { return fputc(character, stream->fp); }

int32_t file_printf(file_handle *stream, const char *format, const char *value) {
    return fprintf(stream->fp, format, value);
}

uint64_t file_tell(file_handle *stream) {
#ifdef __linux__
    return ftello(stream->fp);
#elif defined(__APPLE__) && defined(__MACH__)
    return ftello(stream->fp);
#elif _WIN32
    return _ftelli64(stream->fp);
#elif _WIN64
    return _ftelli64(stream->fp);
#else
    return -1;
#endif
}

int32_t file_close(file_handle *stream) {
    int32_t result = fclose(stream->fp);
    free(stream);
    return result;
}
