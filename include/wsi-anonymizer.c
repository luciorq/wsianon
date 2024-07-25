#include "wsi-anonymizer.h"

int32_t (*handle_format_functions[])(const char **filename, const char *new_label_name, bool keep_macro_image,
                                     bool disable_unlinking, bool do_inplace) = {
    &handle_aperio, &handle_hamamatsu, &handle_mirax, &handle_ventana, &handle_isyntax, &handle_philips_tiff};

struct wsi_data *(*get_wsi_data_functions[])(const char *filename) = {
    &get_wsi_data_aperio,  &get_wsi_data_hamamatsu, &get_wsi_data_mirax,
    &get_wsi_data_ventana, &get_wsi_data_isyntax,   &get_wsi_data_philips_tiff};

int8_t num_of_formats = sizeof(VENDOR_AND_FORMAT_STRINGS) / sizeof(char *);

struct wsi_data *get_wsi_data(const char *filename) {
    if (file_exists(filename)) {
        for (int8_t i = 0; i < num_of_formats - 2; i++) {
            struct wsi_data *wsi_data = get_wsi_data_functions[i](filename);
            if (wsi_data != NULL) {
                return wsi_data;
            }
        }
        // unknown format
        struct wsi_data *wsi_data = malloc(sizeof(*wsi_data));
        wsi_data->format = UNKNOWN;
        return wsi_data;
    } else {
        // invalid format
        struct wsi_data *wsi_data = malloc(sizeof(*wsi_data));
        wsi_data->format = INVALID;
        return wsi_data;
    }
}

int32_t anonymize_wsi_with_result(const char **filename, const char *new_label_name, bool keep_macro_image,
                                  bool disable_unlinking, bool do_inplace) {
    int32_t result = -1;

    struct wsi_data *wsi_data = get_wsi_data(*filename);

    if (wsi_data == NULL) {
        fprintf(stderr, "Error: Unable to collect metadata.\n");
        return result;
    } else if (wsi_data->format == INVALID) {
        fprintf(stderr, "Error: File does not exist or is invalid.\n");
        free(wsi_data);
        return result;
    } else if (wsi_data->format == UNKNOWN) {
        fprintf(stderr, "Error: Unknown file format. Process aborted.\n");
        free(wsi_data);
        return result;
    } else {
        result = handle_format_functions[wsi_data->format](filename, new_label_name, keep_macro_image,
                                                           disable_unlinking, do_inplace);
        free_wsi_data(wsi_data);
        return result;
    }
}

int32_t anonymize_wsi_inplace(const char *filename, const char *new_label_name, bool keep_macro_image,
                              bool disable_unlinking) {
    return anonymize_wsi_with_result(&filename, new_label_name, keep_macro_image, disable_unlinking, true);
}

int32_t anonymize_wsi(const char *filename, const char *new_label_name, bool keep_macro_image, bool disable_unlinking,
                      bool do_inplace) {
    return anonymize_wsi_with_result(&filename, new_label_name, keep_macro_image, disable_unlinking, do_inplace);
}

void free_wsi_data(struct wsi_data *wsi_data) {
    if (wsi_data->metadata_attributes != NULL) {
        for (size_t metadata_id = 0; metadata_id < wsi_data->metadata_attributes->length; metadata_id++) {
            wsi_data->metadata_attributes->attributes[metadata_id]->key = NULL;
            wsi_data->metadata_attributes->attributes[metadata_id]->value = NULL;
            free(wsi_data->metadata_attributes->attributes[metadata_id]);
        }
        free(wsi_data->metadata_attributes->attributes);
        free(wsi_data->metadata_attributes);
    }
    free(wsi_data);
}