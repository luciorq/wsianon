#ifndef HEADER_VENTANA_IO_H
#define HEADER_VENTANA_IO_H

#include "tiff-based-io.h"

static const char BIF[] = "bif";
static const char DOT_BIF[] = ".bif";

// main functions
struct metadata_attribute *get_attribute_ventana(char *buffer, const char *delimiter1, const char *delimiter2);

struct metadata *get_metadata_ventana(file_handle *fp, struct tiff_file *file);

struct wsi_data *get_wsi_data_ventana(const char *filename);

int32_t handle_ventana(const char **filename, const char *new_label_name, bool keep_macro_image, bool disable_unlinking,
                       bool do_inplace);

// additional functions
int64_t get_ventana_label_dir(file_handle *fp, struct tiff_file *file);

int32_t wipe_label_ventana(file_handle *fp, struct tiff_directory *dir, bool big_endian);

int32_t wipe_and_unlink_ventana_directory(file_handle *fp, struct tiff_file *file, int64_t directory, bool big_endian,
                                          bool disable_unlinking);

char *anonymize_xmp_attribute_if_exists(char *buffer, const char *attr, const char *delimiter);

int32_t remove_metadata_in_ventana(file_handle *fp, struct tiff_file *file);

#endif