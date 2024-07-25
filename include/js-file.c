#include "file-api.h"

#include <emscripten.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

struct file_s {
    char *filename;
    char *mode;
    int64_t offset;
    int64_t size;
};

EM_ASYNC_JS(size_t, get_chunk, (void *buffer, size_t size, const char *filename, int64_t offset), {
    const jsFilename = UTF8ToString(filename);
    const anonStream = AnonymizedStream.retrieve(jsFilename);
    const sliceData = await anonStream.getAnonymizedChunk(BigInt(offset), size);
    const sourceView = new Uint8Array(sliceData, 0, sliceData.byteLength);
    const targetView = new Uint8Array(wasmMemory.buffer, buffer, sliceData.byteLength);
    targetView.set(sourceView);
    return sliceData.byteLength;
});

EM_ASYNC_JS(void, set_chunk, (const void *buffer, size_t size, const char *filename, int64_t offset), {
    const sourceView = new Uint8Array(wasmMemory.buffer, buffer, size);
    const targetBuffer = new ArrayBuffer(size);
    const targetView = new Uint8Array(targetBuffer);
    targetView.set(sourceView);
    const jsFilename = UTF8ToString(filename);
    const anonStream = AnonymizedStream.retrieve(jsFilename);
    anonStream.addChanges(targetBuffer, BigInt(offset));
});

EM_ASYNC_JS(int32_t, file_present_in_form, (const char *filename), {
    const jsFilename = UTF8ToString(filename);
    return AnonymizedStream.exists(jsFilename) ? 1 : 0;
});

EM_ASYNC_JS(char *, file_size, (const char *filename), {
    const jsFilename = UTF8ToString(filename);
    const anonStreamSize = AnonymizedStream.retrieve(jsFilename).size.toString();
    const lengthBytes = lengthBytesUTF8(anonStreamSize) + 1;
    var stringOnWasmHeap = _malloc(lengthBytes);
    stringToUTF8(anonStreamSize, stringOnWasmHeap, lengthBytes);
    return stringOnWasmHeap;
});

file_handle *file_open(const char *filename, const char *mode) {
    file_handle *stream = NULL;

    if (file_present_in_form(filename)) {
        stream = (file_handle *)malloc(sizeof(file_handle));
        stream->offset = 0;
        stream->filename = (char *)malloc(strlen(filename) + 1);
        stream->mode = (char *)malloc(strlen(mode) + 1);
        strcpy(stream->filename, filename);
        strcpy(stream->mode, mode);

        // safe method to set file_size for >4GB files
        char *size_str = file_size(filename);
        int64_t size = strtoll(size_str, NULL, 10);
        free(size_str);
        stream->size = size;
    }

    return stream;
}

size_t file_read(void *buffer, size_t element_size, size_t element_count, file_handle *stream) {
    // check if size that is read at once does not exceed limit for array
    size_t size = element_size * element_count;
    if (size < INT_MAX) {
        uint32_t bytes_read = get_chunk(buffer, size, stream->filename, stream->offset);
        stream->offset += bytes_read * element_count;
        return element_count;
    }
    return 0;
}

char *file_gets(char *buffer, int32_t max_count, file_handle *stream) {
    if (stream->offset == stream->size) {
        return NULL;
    }
    if (max_count == 1) {
        buffer[0] = '\0';
        return buffer;
    }

    size_t chunk_size = get_chunk(buffer, max_count - 1, stream->filename, stream->offset);

    char *nl = strchr(buffer, '\n');
    if (nl == NULL) {
        buffer[chunk_size] = '\0';
        stream->offset += chunk_size;
    } else {
        *(nl + 1) = '\0';
        stream->offset = stream->offset + (nl - buffer) + 1;
    }
    return buffer;
}

int32_t file_getc(file_handle *stream) {
    char c;
    if (stream->offset == stream->size) {
        return EOF;
    }
    get_chunk(&c, 1, stream->filename, stream->offset);
    stream->offset += 1;
    return c;
}

int64_t file_seek(file_handle *stream, int64_t offset, int32_t origin) {
    uint64_t new_offset = 0;
    if (origin == SEEK_SET) {
        new_offset = offset;
    } else if (origin == SEEK_CUR) {
        new_offset = stream->offset + offset;
    } else {
        new_offset = stream->size + offset;
    }
    if ((new_offset < 0) || (new_offset > stream->size)) {
        return -1;
    }

    stream->offset = new_offset;
    return 0;
}

size_t file_write(const void *buffer, size_t size, size_t count, file_handle *stream) {
    // TODO: if file not writable, return 0 immediately
    set_chunk(buffer, size * count, stream->filename, stream->offset);
    // TODO: how many bytes were really written -> adapt offset accordingly
    stream->offset += (size * count);
    // TODO: check under which conditions offset is updated for writing (could
    // be that there were differences according to open mode)
    return count;
}

int32_t file_putc(int32_t character, file_handle *stream) {
    // TODO: if file not writable, return 0 immediately
    char c = (char)character; // to be sure to avoid endianess problems
    set_chunk(&c, 1, stream->filename, stream->offset);
    // TODO: check offset update conditions (see comment in file_write)
    stream->offset += 1;
    // TODO: handle failures
    return c;
}

int32_t file_printf(file_handle *stream, const char *format, const char *value) {
    int32_t buffer_size = snprintf(NULL, 0, format, value); // dry run to find out buffer size
    char *buffer = (char *)malloc(buffer_size + 1);
    sprintf(buffer, format, value);
    set_chunk(buffer, buffer_size, stream->filename, stream->offset);
    free(buffer);
    stream->offset += buffer_size;
    // TODO: handle edge cases (partial writes, errors, etc.)
    return buffer_size;
}

uint64_t file_tell(file_handle *stream) { return stream->offset; }

int32_t file_close(file_handle *stream) {
    free(stream->mode);
    free(stream->filename);
    free(stream);
    return 0;
}
