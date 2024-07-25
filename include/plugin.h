/*
 *
 * In order to execute anonymization on an additional format, include this header file in the header
 * file of your new format. For conventional purposes name the newly created header and source file
 * of your format <YOUR_FORMAT>-io.h and <YOUR_FORMAT>-io.c. Implement the functions declared below.
 * You can either keep these functionnames or use them as a template and change them to
 * get_wsi_data_<YOUR_FORMAT>() and handle_<YOUR_FORMAT>() (this is recommenended when you want to implement
 * the anonymization of more than one format). Add your format to the 'VENDOR_AND_FORMAT_STRINGS' variable in
 * wsi-anonymizer.h right before the 'Unknown' entry. Lastly, add &get_wsi_data_format() and &handle_format()
 * (or &get_wsi_data_<YOUR_FORMAT>() and &handle_<YOUR_FORMAT>()) to the *get_wsi_data_functions[] and
 * *handle_format_functions[] arrays in wsi-anonymizer.c.
 *
 */

#ifndef HEADER_PLUGIN_H
#define HEADER_PLUGIN_H

#include <stdbool.h>
#include <stdint.h>

// returns a struct containing all necessary information (label and macro image and metadata) regarding the WSI
struct wsi_data *get_wsi_data_format(const char *filename);

// implements the anonymization for the added format
int32_t handle_format(const char **filename, const char *new_label_name, bool keep_macro_image, bool disable_unlinking,
                      bool do_inplace);

#endif
