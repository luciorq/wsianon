#ifndef HEADER_PHILIPS_IO_H
#define HEADER_PHILIPS_IO_H

#include "philips-based-io.h"
#include "tiff-based-io.h"

static const char DOT_TIFF[] = ".tiff";
static const char TIFF[] = "tiff";

// main functions
struct metadata_attribute *get_attribute_philips_tiff(char *buffer, char *attribute);

struct metadata *get_metadata_philips_tiff(file_handle *fp, struct tiff_file *file);

struct wsi_data *get_wsi_data_philips_tiff(const char *filename);

int32_t handle_philips_tiff(const char **filename, const char *new_label_name, bool keep_macro_image,
                            bool disable_unlinking, bool do_inplace);

// additional functions
int32_t wipe_philips_image_data(file_handle *fp, struct tiff_file *file, char *image_type);

int32_t anonymize_philips_metadata(file_handle *fp, struct tiff_file *file);

#endif