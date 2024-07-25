#ifndef HEADER_APERIO_IO_H
#define HEADER_APERIO_IO_H

#include "tiff-based-io.h"

static const char DOT_SVS[] = ".svs";
static const char SVS[] = "svs";

// main functions
struct metadata_attribute *get_attribute_aperio(char *buffer, const char *attribute_name);

struct metadata *get_metadata_aperio(file_handle *fp, struct tiff_file *file);

struct wsi_data *get_wsi_data_aperio(const char *filename);

int32_t handle_aperio(const char **filename, const char *new_label_name, bool keep_macro_image, bool disable_unlinking,
                      bool do_inplace);

// additional functions
char *override_image_description(char *result, char *delimiter);

int32_t remove_metadata_in_aperio(file_handle *fp, struct tiff_file *file);

int32_t change_macro_image_compression_gt450(file_handle *fp, struct tiff_file *file, int32_t directory);

#endif