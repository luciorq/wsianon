#ifndef HEADER_PHILIPS_BASED_IO_H
#define HEADER_PHILIPS_BASED_IO_H

#include "b64.h"
#include "defines.h"
#include "jpec.h"
#include "utils.h"

char *wipe_section_of_attribute(char *buffer, char *attribute);

char *get_value_from_attribute(char *buffer, char *attribute);

char *anonymize_value_of_attribute(char *buffer, char *attribute);

int32_t *get_height_and_width(const char *image_data);

#endif