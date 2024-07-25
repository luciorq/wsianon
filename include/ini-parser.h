#ifndef HEADER_INI_PARSER_H
#define HEADER_INI_PARSER_H

#include "defines.h"
#include "utils.h"

struct ini_file *read_slidedat_ini_file(const char *path, const char *ini_filename);

const char *get_value_from_ini_file(struct ini_file *ini_file, const char *group_name, const char *key);

int32_t delete_group_form_ini_file(struct ini_file *ini_file, const char *group_name);

int32_t get_group_index_of_ini_file(struct ini_file *ini_file, const char *group_name);

void restructure_levels_in_file(struct ini_file *ini_file, int32_t level_pos_in_layer, int32_t layer_id,
                                struct mirax_file *mirax_file);

void restructure_groups_in_file(struct ini_file *ini_file, struct mirax_level *current_level,
                                struct mirax_level *next_level);

struct ini_group *find_group(struct ini_file *ini_file, const char *group_name);

struct ini_entry *find_entry(struct ini_group *group, const char *key);

const char *anonymize_value_for_group_and_key(struct ini_file *ini_file, const char *group_name, const char *key,
                                              const char c);

const char *anonymize_value_for_group_and_key_with_given_string(struct ini_file *ini_file, const char *group_name,
                                                                const char *key, const char *value);

const char *create_random_slide_id(struct ini_file *ini_file, const char *group_name, const char *key);

void remove_entry_for_group_and_key(struct ini_file *ini_file, const char *group_name, const char *key);

void decrement_value_for_group_and_key(struct ini_file *ini_file, const char *group_name, const char *key);

int32_t write_ini_file(struct ini_file *ini_file, const char *path, const char *filename);

#endif