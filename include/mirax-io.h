#ifndef HEADER_MIRAX_IO_H
#define HEADER_MIRAX_IO_H

#include "defines.h"
#include "ini-parser.h"
#include "tiff-based-io.h"

// main functions
struct metadata_attribute *get_attribute_mirax(struct ini_file *ini_file, const char *group_name,
                                               const char *metadata_key);

struct metadata *get_metadata_mirax(const char *path, struct ini_file *ini_file, const char **data_filenames,
                                    int32_t num_of_datafiles);

struct wsi_data *get_wsi_data_mirax(const char *filename);

int32_t handle_mirax(const char **filename, const char *new_label_name, bool keep_macro_image, bool disable_unlinking,
                     bool do_inplace);

// additional functions
void free_slidedata_ini_file(struct ini_file *ini);

struct mirax_file *get_mirax_file_structure(struct ini_file *ini, int32_t l_count);

struct mirax_level *get_level_by_name(struct mirax_layer **layers, const char *layer_name, const char *level_name);

int32_t *read_data_location(const char *filename, int32_t record, int32_t **position, int32_t **size);

int32_t wipe_level_data(const char *filename, int32_t **offset, int32_t **length, const char *prefix,
                        const char *suffix);

int32_t delete_level(const char *path, const char *index_file, const char **data_files, struct mirax_layer **layers,
                     const char *layer_name, const char *level_name);

int32_t delete_record_from_index_file(const char *filename, int32_t record, int32_t all_records);

const char *duplicate_mirax_filedata(const char *filename, const char *new_label_name, const char *file_extension);

struct mirax_layer *delete_level_by_id(struct mirax_layer *layer, int32_t level_id);

void delete_last_entry_from_ini_file(struct ini_file *ini, struct mirax_file *mirax_file, int32_t layer_id);

void unlink_level(struct ini_file *ini, struct mirax_level *level_to_delete, struct mirax_file *mirax_file);

int32_t replace_slide_id_in_indexdat(const char *path, const char *filename, const char *value, const char *replacement,
                                     int32_t size);

int32_t replace_slide_id_in_datfiles(const char *path, const char **data_files, int32_t length, const char *value,
                                     const char *replacement, int32_t size);

void remove_metadata_in_data_dat(const char *path, const char **data_files, int32_t length);

int32_t wipe_data_in_index_file(const char *path, const char *index_filename, struct mirax_level *level_to_delete,
                                struct mirax_file *mirax_file);

int32_t wipe_delete_unlink(const char *path, struct ini_file *ini, const char *index_filename,
                           struct mirax_file *mirax_file, const char *layer, const char *level_to_delete);

char *strndup(const char *s1, size_t n);

#endif
