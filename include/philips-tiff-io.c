#include "philips-tiff-io.h"

struct metadata_attribute *get_attribute_philips_tiff(char *buffer, char *attribute) {
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

struct metadata *get_metadata_philips_tiff(file_handle *fp, struct tiff_file *file) {
    // all metadata
    static char *METADATA_ATTRIBUTES[] = {PHILIPS_DATETIME_ATT,   PHILIPS_SERIAL_ATT, PHILIPS_SLOT_ATT,
                                          PHILIPS_RACK_ATT,       PHILIPS_OPERID_ATT, PHILIPS_BARCODE_ATT,
                                          PHILIPS_SOURCE_FILE_ATT};

    // initialize metadata_attribute struct
    struct metadata_attribute **attributes =
        malloc(sizeof(**attributes) * sizeof(METADATA_ATTRIBUTES) / sizeof(METADATA_ATTRIBUTES[0]));
    int8_t metadata_id = 0;

    // iterate over directories in tiff file
    for (uint64_t i = 0; i < file->used; i++) {
        struct tiff_directory dir = file->directories[i];
        for (uint64_t j = 0; j < dir.count; j++) {
            struct tiff_entry entry = dir.entries[j];

            // entry with ImageDescription tag contains all metadata
            if (entry.tag == TIFFTAG_IMAGEDESCRIPTION) {
                file_seek(fp, entry.offset, SEEK_SET);
                int32_t entry_size = get_size_of_value(entry.type, &entry.count);

                // read content of ImageDescription into buffer
                char *buffer = (char *)malloc(entry.count * entry_size);
                if (file_read(buffer, entry.count, entry_size, fp) != 1) {
                    fprintf(stderr, "Error: Could not read tag image description.\n");
                    free(buffer);
                    return NULL;
                }

                // checks for all metadata
                for (size_t i = 0; i < sizeof(METADATA_ATTRIBUTES) / sizeof(METADATA_ATTRIBUTES[0]); i++) {
                    if (contains(buffer, METADATA_ATTRIBUTES[i])) {
                        struct metadata_attribute *single_attribute =
                            get_attribute_philips_tiff(buffer, METADATA_ATTRIBUTES[i]);
                        if (single_attribute != NULL) {
                            attributes[metadata_id++] = single_attribute;
                        }
                    }
                }
                free(buffer);
            }
        }
    }
    // add all found metadata
    struct metadata *metadata_attributes = malloc(sizeof(*metadata_attributes));
    metadata_attributes->attributes = attributes;
    metadata_attributes->length = metadata_id;
    return metadata_attributes;
}

struct wsi_data *get_wsi_data_philips_tiff(const char *filename) {
    // gets file extension
    int32_t result = 0;
    const char *ext = get_filename_ext(filename);

    // check for valid file extension
    if (strcmp(ext, TIFF) != 0) {
        return NULL;
    }

    // opens file
    file_handle *fp = file_open(filename, "rb+");

    // checks if file was successfully opened
    if (fp == NULL) {
        fprintf(stderr, "Error: Could not open Philips TIFF file.\n");
        return NULL;
    }

    // checks file details
    bool big_tiff = false;
    bool big_endian = false;
    result = check_file_header(fp, &big_endian, &big_tiff);

    // checks result
    if (result != 0) {
        return NULL;
    }

    // creates tiff file structure
    struct tiff_file *file;
    file = read_tiff_file(fp, big_tiff, false, big_endian);

    // checks result
    if (file == NULL) {
        fprintf(stderr, "Error: Could not read tiff file.\n");
        file_close(fp);
        return NULL;
    }

    // checks if the Software tag starts with Philips
    result = tag_value_contains(fp, file, TIFFTAG_SOFTWARE, "Philips");

    // checks result
    if (result == -1) {
        fprintf(stderr, "Error: Could not find Philips value in Software tag.\n");
        return NULL;
    }

    // gets all metadata
    struct metadata *metadata_attributes = get_metadata_philips_tiff(fp, file);

    // is Philips' TIFF
    struct wsi_data *wsi_data = malloc(sizeof(*wsi_data));
    wsi_data->format = PHILIPS_TIFF;
    wsi_data->filename = filename;
    wsi_data->metadata_attributes = metadata_attributes;

    // cleanup
    free_tiff_file(file);
    file_close(fp);
    return wsi_data;
}

// remove label image and macro image
int32_t wipe_philips_image_data(file_handle *fp, struct tiff_file *file, char *image_type) {
    for (uint64_t i = 0; i < file->used; i++) {
        struct tiff_directory dir = file->directories[i];
        for (uint64_t j = 0; j < dir.count; j++) {
            struct tiff_entry entry = dir.entries[j];
            if (entry.tag == TIFFTAG_IMAGEDESCRIPTION) {
                // get requested image tag from file
                file_seek(fp, entry.offset, SEEK_SET);
                int32_t entry_size = get_size_of_value(entry.type, &entry.count);

                char *buffer = malloc(sizeof(char) * entry_size * entry.count);
                if (file_read(buffer, entry.count, entry_size, fp) != 1) {
                    fprintf(stderr, "Error: Could not read tag image description.\n");
                    return -1;
                }

                char *result = buffer;
                bool rewrite = false;

                // check for label and macro image in image description
                if (contains(result, image_type)) {

                    // get image data string
                    char *rough_image_data = get_string_between_delimiters(result, PHILIPS_OBJECT, image_type);
                    char *refined_image_data =
                        get_string_between_delimiters(rough_image_data, PHILIPS_IMAGE_DATA, PHILIPS_ATT_OPEN);
                    const char *concatenated_str = concat_str(PHILIPS_DELIMITER_STR, PHILIPS_CLOSING_SYMBOL);
                    char *image_data =
                        get_string_between_delimiters(refined_image_data, concatenated_str, PHILIPS_ATT_END);

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

                    // encode new image data and check if string is longer than original string,
                    // replace old base64-encoded string afterwards
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
                    free((void *)concatenated_str);
                    free(image_data);
                    free(white_image);
                    jpec_enc_del(e);
                    free(new_image_data);
                    free(new_result);
                }

                // alter image in image description
                if (rewrite) {
                    strcpy(buffer, result);
                    file_seek(fp, entry.offset, SEEK_SET);
                    if (!file_write(buffer, entry.count, entry_size, fp)) {
                        fprintf(stderr, "Error: Changing image description failed.\n");
                        free(buffer);
                        return -1;
                    }
                }
                free(buffer);
            }
        }
    }
    return 0;
}

// anonymizes metadata from Philips' TIFF file
int32_t anonymize_philips_metadata(file_handle *fp, struct tiff_file *file) {
    for (uint64_t i = 0; i < file->used; i++) {
        struct tiff_directory dir = file->directories[i];
        for (uint64_t j = 0; j < dir.count; j++) {
            struct tiff_entry entry = dir.entries[j];
            if (entry.tag == TIFFTAG_IMAGEDESCRIPTION) {
                // get requested image tag from file
                file_seek(fp, entry.offset, SEEK_SET);
                int32_t entry_size = get_size_of_value(entry.type, &entry.count);

                char *buffer = (char *)malloc(entry.count * entry_size);
                if (file_read(buffer, entry.count, entry_size, fp) != 1) {
                    fprintf(stderr, "Error: Could not read tag image description.\n");
                    return -1;
                }

                bool rewrite = false;
                char *result = buffer;

                // Datetime attribute is substituted with minimum possible value
                if (contains(result, PHILIPS_DATETIME_ATT)) {
                    char *value = get_value_from_attribute(result, PHILIPS_DATETIME_ATT);
                    char *new_result = replace_str(result, value, PHILIPS_MIN_DATETIME);
                    strcpy(result, new_result);
                    rewrite = true;
                    free(value);
                    free(new_result);
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

                // rest of metadata that is replacable with arbitrary value
                static char *METADATA_ATTRIBUTES[] = {PHILIPS_SERIAL_ATT, PHILIPS_OPERID_ATT, PHILIPS_BARCODE_ATT,
                                                      PHILIPS_SOURCE_FILE_ATT};

                // anonymize rest of metadata
                for (size_t i = 0; i < sizeof(METADATA_ATTRIBUTES) / sizeof(METADATA_ATTRIBUTES[0]); i++) {
                    if (contains(result, METADATA_ATTRIBUTES[i])) {
                        char *new_result = anonymize_value_of_attribute(result, METADATA_ATTRIBUTES[i]);
                        strcpy(result, new_result);
                        rewrite = true;
                    }
                }

                // alters image description
                if (rewrite) {
                    strcpy(buffer, result);
                    file_seek(fp, entry.offset, SEEK_SET);
                    if (!file_write(buffer, entry.count, entry_size, fp)) {
                        fprintf(stderr, "Error: changing Image Description failed.\n");
                        free(buffer);
                        return -1;
                    }
                }
                free(buffer);
                return 1;
            }
        }
    }
    return 1;
}

// anonymize Philips' TIFF
int32_t handle_philips_tiff(const char **filename, const char *new_label_name, bool keep_macro_image,
                            bool disable_unlinking, bool do_inplace) {

    fprintf(stdout, "Anonymize Philips TIFF WSI...\n");

    if (!do_inplace) {
        *filename = duplicate_file(*filename, new_label_name, DOT_TIFF);
    }

    file_handle *fp;
    fp = file_open(*filename, "rb+");

    // philips tiff files can be stored as single-tiff TIFF or BigTIFF format
    bool big_tiff = false;
    bool big_endian = false;
    int32_t result = check_file_header(fp, &big_endian, &big_tiff);

    if (result != 0) {
        file_close(fp);
        return result;
    }

    struct tiff_file *file;
    file = read_tiff_file(fp, big_tiff, false, big_endian);

    // if file could not be opened
    if (fp == NULL) {
        return -1;
    }

    // remove LABELIMAGE in ImageDescription XML
    result = wipe_philips_image_data(fp, file, PHILIPS_LABELIMAGE);

    if (result == -1) {
        fprintf(stderr, "Error: Could not wipe LABELIMAGE in ImageDescription XML from file.\n");
    }

    // delete label image that may have been stored in other IFDs under 'Label' in ImageDescriptions
    int32_t label_dir = get_directory_by_tag_and_value(fp, file, TIFFTAG_IMAGEDESCRIPTION, "Label");

    if (label_dir == -1) {
        fprintf(stderr, "Error: Could not find IFD of label image.\n");
    } else {
        struct tiff_directory dir = file->directories[label_dir];
        result = wipe_directory(fp, &dir, false, big_endian, big_tiff, NULL, NULL);

        // if removal did not work
        if (result != 0) {
            free_tiff_file(file);
            file_close(fp);
            return result;
        }
    }

    int32_t macro_dir = -1;

    if (!keep_macro_image) {
        // remove MACROIMAGE in ImageDescription XML
        result = wipe_philips_image_data(fp, file, PHILIPS_MACROIMAGE);

        // if removal did not work
        if (result == -1) {
            fprintf(stderr, "Error: Could not wipe MACROIMAGE in ImageDescription XML from file.\n");
        }
        // delete macro image that may have been stored in other IFDs under 'Macro' in ImageDescriptions
        macro_dir = get_directory_by_tag_and_value(fp, file, TIFFTAG_IMAGEDESCRIPTION, "Macro");

        if (macro_dir == -1) {
            fprintf(stderr, "Error: Could not find IFD of macro image.\n");
        } else {
            struct tiff_directory dir = file->directories[macro_dir];
            result = wipe_directory(fp, &dir, false, big_endian, big_tiff, NULL, NULL);

            // if removal did not work
            if (result != 0) {
                free_tiff_file(file);
                file_close(fp);
                return result;
            }
        }
    }

    // unlinking
    if (!disable_unlinking) {
        fprintf(stderr, "Error: Unlinking in Philips TIFF file currently not supported.\n");
        // ToDo: find out why unlinking of macro directory does not properly work
        // unlink_directory(fp, file, macro_dir, false);
        // unlink_directory(fp, file, label_dir, false);
    }

    // remove metadata
    anonymize_philips_metadata(fp, file);

    // clean up
    free((void *)(*filename));
    free_tiff_file(file);
    file_close(fp);
    return result;
}
