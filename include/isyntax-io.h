#ifndef HEADER_ISYNTAX_IO_H
#define HEADER_ISYNTAX_IO_H

#include "philips-based-io.h"

static const char ISYNTAX_EXT[] = "isyntax";
static const char DOT_ISYNTAX[] = ".isyntax";

// main functions
struct metadata_attribute *get_attribute_isyntax(char *buffer, char *attribute);

struct metadata *get_metadata_isyntax(file_handle *fp, uint64_t header_size);

struct wsi_data *get_wsi_data_isyntax(const char *filename);

int32_t handle_isyntax(const char **filename, const char *new_label_name, bool keep_macro_image, bool disable_unlinking,
                       bool do_inplace);

// additional functions
int32_t anonymize_isyntax_metadata(file_handle *fp, int32_t header_size);

int32_t wipe_isyntax_image_data(file_handle *fp, size_t header_size, char *image_type);

#endif