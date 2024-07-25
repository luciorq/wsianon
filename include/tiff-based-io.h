#ifndef HEADER_TIFF_IO_H
#define HEADER_TIFF_IO_H

#include "defines.h"
#include "utils.h"
#include <inttypes.h>

static const char TIF[] = "tif";
static const char DOT_TIF[] = ".tif";

#define min(a, b)                                                                                                      \
    ({                                                                                                                 \
        __typeof__(a) _a = (a);                                                                                        \
        __typeof__(b) _b = (b);                                                                                        \
        _a < _b ? _a : _b;                                                                                             \
    })

void init_tiff_file(struct tiff_file *file, size_t init_size);

void insert_dir_into_tiff_file(struct tiff_file *file, struct tiff_directory *dir);

void free_tiff_file(struct tiff_file *file);

void fix_byte_order(void *data, int32_t size, int64_t count, bool big_endian);

uint64_t read_uint(file_handle *fp, int32_t size, bool big_endian);

uint32_t get_size_of_value(uint16_t type, uint32_t *count);

uint64_t fix_ndpi_offset(uint64_t directory_offset, uint64_t offset);

void free_tiff_dir_and_entries(struct tiff_directory *tiff_dir, struct tiff_entry *entries, struct tiff_entry *entry);

struct tiff_directory *read_tiff_directory(file_handle *fp, uint64_t *dir_offset, uint64_t *in_pointer_offset,
                                           bool big_tiff, bool ndpi, bool big_endian);

int32_t check_file_header(file_handle *fp, bool *big_endian, bool *big_tiff);

struct tiff_file *read_tiff_file(file_handle *fp, bool big_tiff, bool ndpi, bool big_endian);

int32_t check_prefix(file_handle *fp, const char *prefix);

int32_t wipe_directory(file_handle *fp, struct tiff_directory *dir, bool ndpi, bool big_endian, bool big_tiff,
                       const char *prefix, const char *suffix);

uint32_t *read_pointer32_by_tag(file_handle *fp, struct tiff_directory *dir, int32_t tag, bool ndpi, bool big_endian,
                                int32_t *length);

uint64_t *read_pointer64_by_tag(file_handle *fp, struct tiff_directory *dir, int32_t tag, bool ndpi, bool big_endian,
                                int32_t *length);

int32_t unlink_directory(file_handle *fp, struct tiff_file *file, int32_t current_dir, bool is_ndpi);

int32_t get_aperio_gt450_dir_by_name(struct tiff_file *file, const char *dir_name);

int32_t tag_value_contains(file_handle *fp, struct tiff_file *file, int32_t tag, const char *contains_value);

int32_t get_directory_by_tag_and_value(file_handle *fp, struct tiff_file *file, int32_t tag, const char *value);

const char *duplicate_file(const char *filename, const char *new_label_name, const char *file_extension);

#endif
