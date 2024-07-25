#ifndef HEADER_UTILS_H
#define HEADER_UTILS_H

#include "defines.h"
#include "file-api.h"
#include "time.h"

// string and int operations
char **str_split(char *a_str, const char a_delim);

const char *get_filename_ext(const char *filename);

int32_t file_exists(const char *filename);

char *create_pre_suffixed_char_array(const char x, uint64_t length, const char *prefix, const char *suffix);

char *create_random_string(uint64_t length);

char *create_replacement_string(const char x, uint64_t length);

char *replace_str(const char *original_str, const char *replace_str, const char *with_str);

void replace_str_inplace(char *original_str, const char *replace_str, const char *with_str);

bool starts_with(const char *str, const char *pre);

char *get_string_between_delimiters(const char *buffer, const char *delimiter1, const char *delimiter2);

void remove_leading_spaces(char *str);

const char *concat_str(const char *str1, const char *str2);

const char *concat_path_filename(const char *path, const char *filename);

const char *concat_path_filename_ext(const char *path, const char *filename, const char *ext);

const char *get_filename_from_path(const char *path);

const char *int32_to_str(int32_t integer);

int32_t number_of_digits(int32_t integer);

const char *add_square_brackets(const char *str);

const char *add_equals_sign(const char *str1, const char *str2);

bool contains(const char *str1, const char *str2);

uint64_t iteratively_get_size_of_value(file_handle *fp, char *value, uint64_t step);

const char *concat_wildcard_string_int32(const char *str, int32_t integer);

const char *concat_wildcard_string_m_int32(const char *str, int32_t integer1, int32_t integer2);

int32_t *read_int32(file_handle *fp);

bool assert_value(file_handle *fp, int32_t value);

const char *slice_str(const char *value, size_t start, size_t end);

// file operations
const char *duplicate_file(const char *filename, const char *new_file_name, const char *file_extension);

int32_t copy_file_v2(const char *src, const char *dest);

int32_t copy_directory(const char *src, const char *dest);

// byte operations
bool is_system_big_endian();

uint16_t _swap_uint16(uint16_t value);

uint32_t _swap_uint32(uint32_t value);

uint64_t _swap_uint64(uint64_t value);

int32_t count_contains(const char *str1, const char *str2);

int32_t bytes_to_int(unsigned char *buffer, int32_t size);

#endif