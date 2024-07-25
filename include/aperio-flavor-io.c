#include "aperio-flavor-io.h"

struct metadata_attribute *get_attribute_aperio(char *buffer, const char *attribute_name) {
    const char *prefixed_delimiter = concat_str("|", attribute_name);
    char *value = get_string_between_delimiters(buffer, prefixed_delimiter, "|");
    free((void *)prefixed_delimiter);

    // check if tag is not an empty string
    if (value[0] != '\0') {
        // removes '=' from key and saves it with value in struct
        struct metadata_attribute *single_attribute = malloc(sizeof(*single_attribute));
        single_attribute->key = strdup(attribute_name);
        single_attribute->key[strlen(single_attribute->key) - 3] = '\0';
        single_attribute->value = value;
        return single_attribute;
    }
    free(value);
    return NULL;
}

struct metadata *get_metadata_aperio(file_handle *fp, struct tiff_file *file) {
    // all metadata
    static const char *METADATA_ATTRIBUTES[] = {APERIO_FILENAME_TAG, APERIO_USER_TAG,       APERIO_TIME_TAG,
                                                APERIO_DATE_TAG,     APERIO_SLIDE_TAG,      APERIO_BARCODE_TAG,
                                                APERIO_RACK_TAG,     APERIO_SCANSCOPEID_TAG};

    // initialize metadata_attribute struct
    struct metadata_attribute **attributes =
        malloc(sizeof(**attributes) * sizeof(METADATA_ATTRIBUTES) / sizeof(METADATA_ATTRIBUTES[0]));
    int8_t metadata_id = 0;

    // iterate over directories in tiff file
    for (uint32_t i = 0; i < file->used; i++) {
        struct tiff_directory dir = file->directories[i];
        for (uint32_t j = 0; j < dir.count; j++) {
            struct tiff_entry entry = dir.entries[j];

            // entry with ImageDescription tag contains all metadata
            if (entry.tag == TIFFTAG_IMAGEDESCRIPTION) {
                file_seek(fp, entry.offset, SEEK_SET);
                int32_t entry_size = get_size_of_value(entry.type, &entry.count);

                // read content of ImageDescription into buffer
                char buffer[entry_size * entry.count];
                if (file_read(&buffer, entry.count, entry_size, fp) != 1) {
                    fprintf(stderr, "Error: Could not read tag image description.\n");
                    return NULL;
                }

                // checks for all metadata
                for (size_t i = 0; i < sizeof(METADATA_ATTRIBUTES) / sizeof(METADATA_ATTRIBUTES[0]); i++) {
                    if (contains(buffer, METADATA_ATTRIBUTES[i])) {
                        struct metadata_attribute *single_attribute =
                            get_attribute_aperio(buffer, METADATA_ATTRIBUTES[i]);
                        if (single_attribute != NULL) {
                            attributes[metadata_id++] = single_attribute;
                        }
                    }
                }
            }
        }
    }

    // add all found metadata
    struct metadata *metadata_attributes = malloc(sizeof(*metadata_attributes));
    metadata_attributes->attributes = attributes;
    metadata_attributes->length = metadata_id;
    return metadata_attributes;
}

struct wsi_data *get_wsi_data_aperio(const char *filename) {
    // gets file extension
    int32_t result = 0;
    const char *ext = get_filename_ext(filename);

    // check for valid file extension
    if (strcmp(ext, SVS) != 0 && strcmp(ext, TIF) != 0) {
        return NULL;
    }

    // opens file
    file_handle *fp = file_open(filename, "rb+");

    // checks if file was successfully opened
    if (fp == NULL) {
        fprintf(stderr, "Error: Could not open Aperio file.\n");
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

    // checks tag value in order to determine if file is actually Aperio
    result = tag_value_contains(fp, file, TIFFTAG_IMAGEDESCRIPTION, "Aperio");

    // checks result
    if (result == -1) {
        fprintf(stderr, "Error: Could not find aperio label directory.\n");
        return NULL;
    }

    // gets all metadata
    struct metadata *metadata_attributes = get_metadata_aperio(fp, file);

    // is Aperio
    struct wsi_data *wsi_data = malloc(sizeof(*wsi_data));
    wsi_data->format = APERIO;
    wsi_data->metadata_attributes = metadata_attributes;

    // cleanup
    free_tiff_file(file);
    file_close(fp);
    return wsi_data;
}

// searches for tags in image description data and replaces its values with equal amount of X's
char *override_image_description(char *result, char *delimiter) {
    const char *prefixed_delimiter = concat_str("|", delimiter);
    const char *value = get_string_between_delimiters(result, prefixed_delimiter, "|");
    // check if value is not an empty string
    if (value[0] != '\0') {
        char *replacement = create_replacement_string('X', strlen(value));
        const char *to_be_replaced = concat_str(prefixed_delimiter, value);
        const char *full_replacement = concat_str(prefixed_delimiter, replacement);
        result = replace_str(result, to_be_replaced, full_replacement);
        free((void *)(to_be_replaced));
        free((void *)(full_replacement));
        free(replacement);
        free((void *)(value));
        free((void *)(prefixed_delimiter));
        return result;
    }
    free((void *)(value));
    free((void *)(prefixed_delimiter));
    return NULL;
}

// TODO: make use of get_metadata_aperio function
// removes all metadata
int32_t remove_metadata_in_aperio(file_handle *fp, struct tiff_file *file) {
    for (uint32_t i = 0; i < file->used; i++) {
        struct tiff_directory dir = file->directories[i];
        for (uint32_t j = 0; j < dir.count; j++) {
            struct tiff_entry entry = dir.entries[j];
            if (entry.tag == TIFFTAG_IMAGEDESCRIPTION) {
                // get requested image tag from file
                file_seek(fp, entry.offset, SEEK_SET);
                int32_t entry_size = get_size_of_value(entry.type, &entry.count);

                char buffer[entry_size * entry.count];
                if (file_read(&buffer, entry.count, entry_size, fp) != 1) {
                    fprintf(stderr, "Error: Could not read tag image description.\n");
                    return -1;
                }

                bool rewrite = false;
                char *result = buffer;

                // all metadata that is replaced with default values
                static char *METADATA_ATTRIBUTE_KEYS[] = {APERIO_DATE_TAG, APERIO_TIME_TAG, APERIO_SLIDE_TAG};

                // default replacement values
                static char *METADATA_REPLACEMENT_VALUES[] = {APERIO_MIN_DATE, MIN_TIME, MIN_POS};

                for (size_t i = 0; i < sizeof(METADATA_ATTRIBUTE_KEYS) / sizeof(METADATA_ATTRIBUTE_KEYS[0]); i++) {
                    if (contains(result, METADATA_ATTRIBUTE_KEYS[i])) {
                        const char *prefixed_delimiter = concat_str("|", METADATA_ATTRIBUTE_KEYS[i]);
                        const char *value = get_string_between_delimiters(result, prefixed_delimiter, "|");
                        if (value[0] != '\0') {
                            char *new_result = replace_str(result, value, METADATA_REPLACEMENT_VALUES[i]);
                            strcpy(result, new_result);
                            free(new_result);
                            rewrite = true;
                        }
                        free((void *)(value));
                        free((void *)(prefixed_delimiter));
                    }
                }

                // all metadata that can be replaced with X's
                static char *METADATA_ATTRIBUTES[] = {APERIO_FILENAME_TAG, APERIO_USER_TAG, APERIO_BARCODE_TAG,
                                                      APERIO_SCANSCOPEID_TAG, APERIO_RACK_TAG};

                for (size_t i = 0; i < sizeof(METADATA_ATTRIBUTES) / sizeof(METADATA_ATTRIBUTES[0]); i++) {
                    if (contains(result, METADATA_ATTRIBUTES[i])) {
                        char *new_result = override_image_description(result, METADATA_ATTRIBUTES[i]);
                        // in case the metadata exists but no value was found
                        if (new_result != NULL) {
                            strcpy(result, new_result);
                            free(new_result);
                            rewrite = true;
                        }
                    }
                }

                if (rewrite == true) {
                    file_seek(fp, entry.offset, SEEK_SET);
                    if (file_write(result, entry.count, entry_size, fp) != 1) {
                        fprintf(stderr, "Error: Could not overwrite image description.\n");
                        return -1;
                    }
                }
            }
        }
    }
    return 1;
}

// macro image for gt450 needs to be treated differently because it is JPEG encoded.
// therefore we need to convert it to LZW compression
int32_t change_macro_image_compression_gt450(file_handle *fp, struct tiff_file *file, int32_t directory) {
    struct tiff_directory dir = file->directories[directory];
    for (uint32_t i = 0; i < dir.count; i++) {
        struct tiff_entry entry = dir.entries[i];
        if (entry.tag == TIFFTAG_COMPRESSION) {
            if (file_seek(fp, entry.start + 12, SEEK_SET)) {
                fprintf(stderr, "Error: Failed to seek to offset %" PRIu64 ".\n", entry.offset);
                continue;
            }
            uint64_t lzw_com = COMPRESSION_LZW;
            if (!file_write(&lzw_com, 1, sizeof(uint64_t), fp)) {
                fprintf(stderr, "Error: Wiping image data failed.\n");
                return -1;
            }
            break;
        }
    }
    return 0;
}

// anonymizes aperio file
int32_t handle_aperio(const char **filename, const char *new_label_name, bool keep_macro_image, bool disable_unlinking,
                      bool do_inplace) {
    fprintf(stdout, "Anonymize Aperio WSI...\n");

    // gets file extension
    const char *ext = get_filename_ext(*filename);

    // checks for valid file extension
    bool is_svs = strcmp(ext, SVS) == 0;

    // duplicate file
    if (!do_inplace) {
        // check if filename is svs or tif here
        *filename = duplicate_file(*filename, new_label_name, is_svs ? DOT_SVS : DOT_TIF);
    }

    // open file
    file_handle *fp;
    fp = file_open(*filename, "rb+");

    // checks file details
    bool big_tiff = false;
    bool big_endian = false;
    int32_t result = check_file_header(fp, &big_endian, &big_tiff);

    // check result
    if (result != 0) {
        file_close(fp);
        return result;
    }

    // creates tiff file structure
    struct tiff_file *file;
    file = read_tiff_file(fp, big_tiff, false, big_endian);

    // check result
    if (file == NULL) {
        fprintf(stderr, "Error: Could not read tiff file.\n");
        file_close(fp);
        return -1;
    }

    // if the slide at hand is produced by a GT450, the compression needs to be converted
    int32_t _is_aperio_gt450 = tag_value_contains(fp, file, TIFFTAG_IMAGEDESCRIPTION, "GT450");

    // delete label image
    int32_t label_dir = 0;
    if (_is_aperio_gt450 == 1) {
        label_dir = get_aperio_gt450_dir_by_name(file, LABEL);
    } else {
        label_dir = get_directory_by_tag_and_value(fp, file, TIFFTAG_IMAGEDESCRIPTION, LABEL);
    }

    // check if label directory could be retrieved
    if (label_dir == -1) {
        fprintf(stderr, "Error: Could not find IFD of label image in Aperio format scanned by GT450.\n");
        free_tiff_file(file);
        file_close(fp);
        return -1;
    }

    // get directoriy
    struct tiff_directory dir = file->directories[label_dir];

    // check for KFBIO value in image description, since the compression type for KFBIO produced Aperio formats differs
    int32_t _is_aperio_kfbio = tag_value_contains(fp, file, TIFFTAG_IMAGEDESCRIPTION, "KFBIO");
    if (_is_aperio_kfbio == 1) {
        result = wipe_directory(fp, &dir, false, big_endian, big_tiff, NULL, NULL);
    } else {
        result = wipe_directory(fp, &dir, false, big_endian, big_tiff, LZW_CLEARCODE, NULL);
    }

    // check for successful wipe of directory
    if (result != 0) {
        free_tiff_file(file);
        file_close(fp);
        return result;
    }

    // delete macro image
    int32_t macro_dir = -1;
    if (!keep_macro_image) {
        if (_is_aperio_gt450 == 1) {
            macro_dir = get_aperio_gt450_dir_by_name(file, MACRO);
        } else {
            macro_dir = get_directory_by_tag_and_value(fp, file, TIFFTAG_IMAGEDESCRIPTION, MACRO);
        }

        if (macro_dir == -1) {
            fprintf(stderr, "Error: Could not find IFD of macro image.\n");
            free_tiff_file(file);
            file_close(fp);
            return -1;
        }

        struct tiff_directory dir = file->directories[macro_dir];
        result = wipe_directory(fp, &dir, false, big_endian, big_tiff, NULL, NULL);

        if (_is_aperio_gt450 == 1) {
            result = change_macro_image_compression_gt450(fp, file, macro_dir);
        }

        if (result != 0) {
            free_tiff_file(file);
            file_close(fp);
            return result;
        }
    }

    // remove all metadata
    remove_metadata_in_aperio(fp, file);

    // unlink directories
    if (!disable_unlinking) {
        unlink_directory(fp, file, label_dir, false);
        unlink_directory(fp, file, macro_dir, false);
    }

    // clean up
    free((void *)(*filename));
    free_tiff_file(file);
    file_close(fp);
    return result;
}