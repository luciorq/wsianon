#ifndef HEADER_WSI_ANONYMIZER_H
#define HEADER_WSI_ANONYMIZER_H

#include "aperio-flavor-io.h"
#include "defines.h"
#include "hamamatsu-io.h"
#include "isyntax-io.h"
#include "mirax-io.h"
#include "philips-tiff-io.h"
#include "plugin.h"
#include "ventana-io.h"

static const char *VENDOR_AND_FORMAT_STRINGS[] = {"Aperio",          "Hamamatsu",    "3DHistech (Mirax)", "Ventana",
                                                  "Philips iSyntax", "Philips TIFF", "Unknown",           "Invalid"};

extern struct wsi_data *get_wsi_data(const char *filename);

extern int32_t anonymize_wsi_inplace(const char *filename, const char *new_label_name, bool keep_macro_image,
                                     bool disable_unlinking);

extern int32_t anonymize_wsi(const char *filename, const char *new_label_name, bool keep_macro_image,
                             bool disable_unlinking, bool do_inplace);

extern void free_wsi_data(struct wsi_data *wsi_data);

#endif
