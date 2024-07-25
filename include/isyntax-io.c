#include "isyntax-io.h"
#include <inttypes.h>

struct metadata_attribute *get_attribute_isyntax(char *buffer, char *attribute) {
    char *value = get_value_from_attribute(buffer, attribute);
    // check if value of attribute is not an empty string
    if (value[0] != '\0') {
        // removes '=' from key and saves it with value in struct
        struct metadata_attribute *single_attribute = malloc(sizeof(*single_attribute));
        if (contains(attribute, "=")) {
            char *pos_of_char = strchr(attribute, '=');
            attribute = pos_of_char + 2;
        }
        single_attribute->key = strdup(attribute);
        single_attribute->value = strdup(value);
        free(value);
        return single_attribute;
    }
    free(value);
    return NULL;
}

struct metadata *get_metadata_isyntax(file_handle *fp, uint64_t header_size) {
    // all metadata
    static char *METADATA_ATTRIBUTES[] = {PHILIPS_DATETIME_ATT, PHILIPS_SERIAL_ATT, PHILIPS_SLOT_ATT,
                                          PHILIPS_RACK_ATT,     PHILIPS_OPERID_ATT, PHILIPS_BARCODE_ATT};

    // initialize metadata_attribute struct
    struct metadata_attribute **attributes =
        malloc(sizeof(**attributes) * sizeof(METADATA_ATTRIBUTES) / sizeof(METADATA_ATTRIBUTES[0]));
    int8_t metadata_id = 0;

    // read content of XML header into buffer
    char *buffer = malloc(header_size);
    file_seek(fp, 0, SEEK_SET);
    if (file_read(buffer, header_size, 1, fp) != 1) {
        free(buffer);
        fprintf(stderr, "Error: Could not read XML header of iSyntax file.\n");
        return NULL;
    }

    // checks for all metadata
    for (size_t i = 0; i < sizeof(METADATA_ATTRIBUTES) / sizeof(METADATA_ATTRIBUTES[0]); i++) {
        if (contains(buffer, METADATA_ATTRIBUTES[i])) {
            struct metadata_attribute *single_attribute = get_attribute_isyntax(buffer, METADATA_ATTRIBUTES[i]);
            if (single_attribute != NULL) {
                attributes[metadata_id++] = single_attribute;
            }
        }
    }

    // add all found metadata
    free(buffer);
    struct metadata *metadata_attributes = malloc(sizeof(*metadata_attributes));
    metadata_attributes->attributes = attributes;
    metadata_attributes->length = metadata_id;
    return metadata_attributes;
}

struct wsi_data *get_wsi_data_isyntax(const char *filename) {
    // gets file extension
    uint64_t result = 0;
    const char *ext = get_filename_ext(filename);

    // check for valid file extension
    if (strcmp(ext, ISYNTAX_EXT) != 0) {
        return NULL;
    }

    // opens file
    file_handle *fp = file_open(filename, "rb+");

    // check if file was successfully opened
    if (fp == NULL) {
        fprintf(stderr, "Error: Could not open iSyntax file.\n");
        return NULL;
    }

    // checks for ISYNTAX_ROOTNODE in order to determine if file is actually iSyntax
    result = iteratively_get_size_of_value(fp, ISYNTAX_ROOTNODE, ISYNTAX_APPROX_HEADER_SIZE);

    // checks result
    if (result == 0) {
        fprintf(stderr, "Error: Could not find root node in iSyntax file.\n");
        return NULL;
    }

    // get header size
    uint64_t header_size = iteratively_get_size_of_value(fp, ISYNTAX_EOT, ISYNTAX_APPROX_EOT_SIZE);

    // checks result
    if (header_size == 0) {
        fprintf(stderr, "Error: Unable to determine XML header size.\n");
        return NULL;
    }

    // gets all metadata
    struct metadata *metadata_attributes = get_metadata_isyntax(fp, header_size);

    // is iSyntax
    struct wsi_data *wsi_data = malloc(sizeof(*wsi_data));
    wsi_data->format = PHILIPS_ISYNTAX;
    wsi_data->filename = filename;
    wsi_data->metadata_attributes = metadata_attributes;

    // cleanup
    file_close(fp);
    return wsi_data;
}

// TODO: make use of get_wsi_data
// anonymizes metadata from iSyntax file
int32_t anonymize_isyntax_metadata(file_handle *fp, int32_t header_size) {

    // gets only XML header
    char *buffer = (char *)malloc(header_size);

    file_seek(fp, 0, SEEK_SET);

    if (file_read(buffer, header_size, 1, fp) != 1) {
        free(buffer);
        fprintf(stderr, "Error: Could not read iSyntax file.\n");
        return -1;
    }

    char *result = buffer;
    bool rewrite = false;

    // Datetime attribute is substituted with minimum possible value
    if (contains(result, PHILIPS_DATETIME_ATT)) {
        char *value = get_value_from_attribute(result, PHILIPS_DATETIME_ATT);
        char *new_result = replace_str(result, value, PHILIPS_MIN_DATETIME);
        strcpy(result, new_result);
        free(new_result);
        free(value);
        rewrite = true;
    }

    // Slot and Rack Number value in metadata is replaced by blank spaces
    static char *METADATA_NUMBER[] = {PHILIPS_SLOT_ATT, PHILIPS_RACK_ATT};

    for (size_t i = 0; i < sizeof(METADATA_NUMBER) / sizeof(METADATA_NUMBER[0]); i++) {
        if (contains(result, METADATA_NUMBER[i])) {
            char *new_result = wipe_section_of_attribute(result, METADATA_NUMBER[i]);
            strcpy(result, new_result);
            free(new_result);
            rewrite = true;
        }
    }

    // rest of metadata
    static char *METADATA_ATTRIBUTES[] = {PHILIPS_SERIAL_ATT, PHILIPS_OPERID_ATT, PHILIPS_BARCODE_ATT};

    // checks for rest of metadata and replaces it with arbitrary value
    for (size_t i = 0; i < sizeof(METADATA_ATTRIBUTES) / sizeof(METADATA_ATTRIBUTES[0]); i++) {
        if (contains(result, METADATA_ATTRIBUTES[i])) {
            char *new_result = anonymize_value_of_attribute(result, METADATA_ATTRIBUTES[i]);
            strcpy(result, new_result);
            rewrite = true;
        }
    }

    // alters iSyntax file
    if (rewrite) {
        strcpy(buffer, result);
        file_seek(fp, 0, SEEK_SET);
        if (!file_write(buffer, header_size, 1, fp)) {
            fprintf(stderr, "Error: Changing XML Header failed.\n");
            free(buffer);
            return -1;
        }
    }

    free(buffer);
    return 1;
}

// remove label image and macro image
int32_t wipe_isyntax_image_data(file_handle *fp, size_t header_size, char *image_type) {

    // gets only the xml header
    char *buffer = (char *)malloc(header_size);

    file_seek(fp, 0, SEEK_SET);

    if (file_read(buffer, header_size, 1, fp) != 1) {
        fprintf(stderr, "Error: Could not read iSyntax file.\n");
        return -1;
    }

    // handle bigger overhead when using malloc in WASM
    if (strlen(buffer) > header_size) {
        buffer[header_size - 1] = '\0';
    }

    char *result = buffer;
    bool rewrite = false;

    if (contains(result, image_type)) {

        // get image data string
        char *rough_image_data = get_string_between_delimiters(result, image_type, PHILIPS_OBJECT);
        char *refined_image_data = get_string_between_delimiters(rough_image_data, PHILIPS_IMAGE_DATA, ISYNTAX_DATA);
        const char *concatenated_str = concat_str(PHILIPS_DELIMITER_STR, PHILIPS_CLOSING_SYMBOL);
        char *image_data = get_string_between_delimiters(refined_image_data, concatenated_str, PHILIPS_ATT_END);

        // set height and width to 1
        int32_t height = 1;
        int32_t width = 1;

        // alloc with height and width and fill with 255 for a white image
        unsigned char *white_image = (unsigned char *)malloc((height * width) * sizeof(unsigned char));
        memset(white_image, 255, height * width);

        // create white jpg image
        jpec_enc_t *e = jpec_enc_new(white_image, width, height);
        int32_t len;
        const uint8_t *jpeg = jpec_enc_run(e, &len);

        // encode new image data and check if string is longer than original string, replace old
        // base64-encoded string afterwards
        char *new_image_data = (char *)b64_encode(jpeg, len);
        if (strlen(new_image_data) > strlen(image_data)) {
            new_image_data[strlen(image_data)] = '\0';
        }

        char *new_result = replace_str(result, image_data, new_image_data);
        strcpy(result, new_result);
        rewrite = true;

        // free all memory
        free(rough_image_data);
        free(refined_image_data);
        free((char *)concatenated_str);
        free(image_data);
        free(white_image);
        jpec_enc_del(e);
        free(new_image_data);
        free(new_result);
    }

    // alter XML header
    if (rewrite) {
        strcpy(buffer, result);
        file_seek(fp, 0, SEEK_SET);
        if (!file_write(buffer, header_size, 1, fp)) {
            fprintf(stderr, "Error: changing XML Header failed.\n");
            result = NULL;
            free(buffer);
            return -1;
        }
    }
    result = NULL;
    free(buffer);
    return 0;
}

// anonymize iSyntax file
int32_t handle_isyntax(const char **filename, const char *new_label_name, bool keep_macro_image, bool disable_unlinking,
                       bool do_inplace) {

    if (disable_unlinking) {
        fprintf(stderr, "Error: Cannot disable unlinking in iSyntax file.\n");
    }

    fprintf(stdout, "Anonymize iSyntax WSI...\n");

    if (!do_inplace) {
        *filename = duplicate_file(*filename, new_label_name, DOT_ISYNTAX);
    }

    file_handle *fp = file_open(*filename, "rb+");

    // if file could not be opened
    if (fp == NULL) {
        fprintf(stderr, "Error: Could not open iSyntax file.\n");
        return -1;
    }

    uint64_t header_size = iteratively_get_size_of_value(fp, ISYNTAX_EOT, ISYNTAX_APPROX_EOT_SIZE);

    // remove label image
    int32_t result = wipe_isyntax_image_data(fp, header_size, PHILIPS_LABELIMAGE);

    if (result == -1) {
        fprintf(stderr, "Error: Could not wipe label image from file.\n");
        file_close(fp);
        return result;
    }

    // remove macro image
    if (!keep_macro_image) {
        result = wipe_isyntax_image_data(fp, header_size, PHILIPS_MACROIMAGE);

        if (result == -1) {
            fprintf(stderr, "Error: Could not wipe macro image from file.\n");
        }
    }

    anonymize_isyntax_metadata(fp, header_size);

    // clean up
    free((char *)(*filename));
    file_close(fp);
    return result;
}