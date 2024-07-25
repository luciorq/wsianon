#include "ventana-io.h"

struct metadata_attribute *get_attribute_ventana(char *buffer, const char *delimiter1, const char *delimiter2) {
    char *value = get_string_between_delimiters(buffer, delimiter1, delimiter2);
    // check if tag is not an empty string
    if (value[0] != '\0') {
        // removes '=' from key and saves it with value in struct
        struct metadata_attribute *single_attribute = malloc(sizeof(*single_attribute));
        single_attribute->key = strdup(delimiter1);
        single_attribute->key[strlen(single_attribute->key) - 2] = '\0';
        single_attribute->value = strdup(value);
        free(value);
        return single_attribute;
    }
    free(value);
    return NULL;
}

struct metadata *get_metadata_ventana(file_handle *fp, struct tiff_file *file) {
    // all metadata with double quotes
    static const char *METADATA_ATTRIBUTES[] = {VENTANA_BASENAME_ATT, VENTANA_FILENAME_ATT,  VENTANA_UNITNUMBER_ATT,
                                                VENTANA_USERNAME_ATT, VENTANA_BUILDDATE_ATT, VENTANA_BARCODE1D_ATT,
                                                VENTANA_BARCODE2D_ATT};

    // all metadata with single quotes
    static const char *METADATA_ATTRIBUTES_2[] = {
        VENTANA_BASENAME_ATT_2,  VENTANA_FILENAME_ATT_2,  VENTANA_UNITNUMBER_ATT_2, VENTANA_USERNAME_ATT_2,
        VENTANA_BUILDDATE_ATT_2, VENTANA_BARCODE1D_ATT_2, VENTANA_BARCODE2D_ATT_2};

    // initialize metadata_attribute struct
    struct metadata_attribute **attributes =
        malloc(sizeof(**attributes) * (sizeof(METADATA_ATTRIBUTES) / sizeof(METADATA_ATTRIBUTES[0]) +
                                       sizeof(METADATA_ATTRIBUTES) / sizeof(METADATA_ATTRIBUTES[0])));
    int8_t metadata_id = 0;

    // iterate over directories in tiff file
    for (uint64_t i = 0; i < file->used; i++) {
        struct tiff_directory dir = file->directories[i];
        for (uint64_t j = 0; j < dir.count; j++) {
            struct tiff_entry entry = dir.entries[j];

            // entry with XMP Tag contains metadata
            if (entry.tag == TIFFTAG_XMP) {
                file_seek(fp, entry.offset, SEEK_SET);
                int32_t entry_size = get_size_of_value(entry.type, &entry.count);

                // read content of XMP into buffer
                char *buffer = malloc(entry_size * entry.count);
                if (file_read(buffer, entry.count, entry_size, fp) != 1) {
                    fprintf(stderr, "Error: Could not read XMP Tag.\n");
                    free(buffer);
                    return NULL;
                }

                // checks for all metadata with double quotes
                for (size_t k = 0; k < sizeof(METADATA_ATTRIBUTES) / sizeof(METADATA_ATTRIBUTES[0]); k++) {
                    if (contains(buffer, METADATA_ATTRIBUTES[k])) {
                        struct metadata_attribute *single_attribute =
                            get_attribute_ventana(buffer, METADATA_ATTRIBUTES[k], "\"");
                        if (single_attribute != NULL) {
                            attributes[metadata_id++] = single_attribute;
                        }
                    }
                }

                // checks for all metadata with double quotes
                for (size_t k = 0; k < sizeof(METADATA_ATTRIBUTES_2) / sizeof(METADATA_ATTRIBUTES_2[0]); k++) {
                    if (contains(buffer, METADATA_ATTRIBUTES_2[k])) {
                        struct metadata_attribute *single_attribute =
                            get_attribute_ventana(buffer, METADATA_ATTRIBUTES_2[k], "\'");
                        if (single_attribute != NULL) {
                            attributes[metadata_id++] = single_attribute;
                        }
                    }
                }
                free(buffer);
            }

            if (entry.tag == TIFFTAG_DATETIME) {
                file_seek(fp, entry.offset, SEEK_SET);
                int32_t entry_size = get_size_of_value(entry.type, &entry.count);
                char *buffer = malloc(entry_size * entry.count);
                if (file_read(buffer, entry.count, entry_size, fp) != 1) {
                    fprintf(stderr, "Error: Could not read DATE_TIME Tag.\n");
                    free(buffer);
                    return NULL;
                }
                // add metadata
                struct metadata_attribute *single_attribute = malloc(sizeof(*single_attribute));
                single_attribute->key = strdup("Datetime");
                single_attribute->value = strdup(buffer);
                attributes[metadata_id++] = single_attribute;
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

struct wsi_data *get_wsi_data_ventana(const char *filename) {
    // gets file extension
    int32_t result = 0;
    const char *ext = get_filename_ext(filename);

    // check for valid file extension
    if (strcmp(ext, BIF) != 0 && strcmp(ext, TIF) != 0) {
        return NULL;
    }

    // opens file
    file_handle *fp = file_open(filename, "rb+");

    // checks if file was successfully opened
    if (fp == NULL) {
        fprintf(stderr, "Error: Could not open Ventana file.\n");
        return NULL;
    }

    // checks file details
    bool big_tiff = false;
    bool big_endian = false;
    result = check_file_header(fp, &big_endian, &big_tiff);

    // checks result
    if (!big_tiff || result != 0) {
        fprintf(stderr, "Error: Not a valid Ventana file.\n");
        file_close(fp);
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

    // checks tag value in order to if file is actually Ventana
    result = tag_value_contains(fp, file, TIFFTAG_XMP, "iScan");

    // checks result
    if (result == -1) {
        fprintf(stderr, "Error: Could not find XMP tag.\n");
        file_close(fp);
        return NULL;
    }

    // gets all metadata
    struct metadata *metadata_attributes = get_metadata_ventana(fp, file);

    // is Ventana
    struct wsi_data *wsi_data = malloc(sizeof(*wsi_data));
    wsi_data->format = VENTANA;
    wsi_data->filename = filename;
    wsi_data->metadata_attributes = metadata_attributes;

    // clean up
    free_tiff_file(file);
    file_close(fp);
    return wsi_data;
}

// gets the label directory of ventana file
int64_t get_ventana_label_dir(file_handle *fp, struct tiff_file *file) {

    for (uint64_t i = 0; i < file->used; i++) {

        struct tiff_directory dir = file->directories[i];
        for (uint64_t j = 0; j < dir.count; j++) {
            struct tiff_entry entry = dir.entries[j];
            if (entry.tag == TIFFTAG_IMAGEDESCRIPTION) {
                file_seek(fp, entry.offset, SEEK_SET);
                int32_t entry_size = get_size_of_value(entry.type, &entry.count);

                char *buffer = malloc(entry_size * entry.count);
                if (file_read(buffer, entry.count, entry_size, fp) != 1) {
                    fprintf(stderr, "Error: Could not read image description.\n");
                    free(buffer);
                    return -1;
                }

                if (contains(buffer, "Label")) {
                    free(buffer);
                    return i;
                }
                free(buffer);
            }
        }
    }
    return -1;
}

// wipes the label directory of ventana file by replacing bytes with zeros
int32_t wipe_label_ventana(file_handle *fp, struct tiff_directory *dir, bool big_endian) {
    int32_t offset_tag = TIFFTAG_TILEOFFSETS;
    int32_t byte_count_tag = TIFFTAG_TILEBYTECOUNTS;

    for (uint64_t i = 0; i < dir->count; i++) {
        struct tiff_entry entry = dir->entries[i];
        // the label image might be saved strip-based instead of tile-based
        // so we have to search for the respective tiff tags to distinguish
        if (entry.tag == TIFFTAG_STRIPOFFSETS) {
            offset_tag = TIFFTAG_STRIPOFFSETS;
            byte_count_tag = TIFFTAG_STRIPBYTECOUNTS;
            break;
        }
    }

    int32_t size_offsets;
    int32_t size_lengths;
    uint64_t *strip_offsets = read_pointer64_by_tag(fp, dir, offset_tag, false, big_endian, &size_offsets);
    uint64_t *strip_lengths = read_pointer64_by_tag(fp, dir, byte_count_tag, false, big_endian, &size_lengths);

    if (strip_offsets == NULL || strip_lengths == NULL) {
        fprintf(stderr, "Error: Could not retrieve strip offset and length.\n");
        return -1;
    }

    if (size_offsets != size_lengths) {
        fprintf(stderr, "Error: Length of strip offsets and lengths are not matching.\n");
        free(strip_offsets);
        free(strip_lengths);
        return -1;
    }

    for (int32_t i = 0; i < size_offsets; i++) {
        file_seek(fp, strip_offsets[i], SEEK_SET);

        char *strip = create_pre_suffixed_char_array('0', strip_lengths[i], NULL, NULL);
        if (!file_write(strip, 1, strip_lengths[i], fp)) {
            fprintf(stderr, "Error: Wiping image data failed.\n");
            free(strip_offsets);
            free(strip_lengths);
            free(strip);
            return -1;
        }
        free(strip);
    }
    free(strip_offsets);
    free(strip_lengths);
    return 0;
}

// wipes and unlinks directory
int32_t wipe_and_unlink_ventana_directory(file_handle *fp, struct tiff_file *file, int64_t directory, bool big_endian,
                                          bool disable_unlinking) {
    // surpress compiler warning; remove when unlinking is implemented
    UNUSED(disable_unlinking);

    struct tiff_directory dir = file->directories[directory];

    int32_t result = wipe_label_ventana(fp, &dir, big_endian);

    // ToDo: check if unlinking for overview image (IFD 0) for .bif files is possible
    /*
    if (result != -1 && !disable_unlinking) {
        result = unlink_directory(fp, file, directory, false);
    }
    */

    return result;
}

char *anonymize_xmp_attribute_if_exists(char *buffer, const char *attr, const char *delimiter) {
    char *value = get_string_between_delimiters(buffer, attr, delimiter);
    // check if tag is not an empty string
    if (value[0] != '\0') {
        char *replacement = create_replacement_string(' ', strlen(value));
        char *result = replace_str(buffer, value, replacement);
        free(replacement);
        free(value);
        return result;
    }
    return NULL;
}

// TODO: make use of wsi_data struct
// anonymizes metadata in XMP Tags of ventana file
int32_t remove_metadata_in_ventana(file_handle *fp, struct tiff_file *file) {

    for (uint64_t i = 0; i < file->used; i++) {
        struct tiff_directory dir = file->directories[i];
        for (uint64_t j = 0; j < dir.count; j++) {
            struct tiff_entry entry = dir.entries[j];
            // searches for XMP Tag in all directories and removes metadata in it
            if (entry.tag == TIFFTAG_XMP) {
                file_seek(fp, entry.offset, SEEK_SET);
                int32_t entry_size = get_size_of_value(entry.type, &entry.count);
                char *buffer = malloc(entry_size * entry.count);
                if (file_read(buffer, entry.count, entry_size, fp) != 1) {
                    fprintf(stderr, "Error: Could not read XMP Tag.\n");
                    free(buffer);
                    return -1;
                }

                char *result = buffer;
                bool rewrite = false;

                // all metadata with double quotes
                const char *METADATA_ATTRIBUTES[] = {
                    VENTANA_BASENAME_ATT,  VENTANA_FILENAME_ATT,  VENTANA_UNITNUMBER_ATT, VENTANA_USERNAME_ATT,
                    VENTANA_BUILDDATE_ATT, VENTANA_BARCODE1D_ATT, VENTANA_BARCODE2D_ATT};

                for (size_t k = 0; k < sizeof(METADATA_ATTRIBUTES) / sizeof(METADATA_ATTRIBUTES[0]); k++) {
                    if (contains(result, METADATA_ATTRIBUTES[k])) {
                        char *new_result = anonymize_xmp_attribute_if_exists(result, METADATA_ATTRIBUTES[k], "\"");
                        if (new_result != NULL) {
                            strcpy(result, new_result);
                            free(new_result);
                            rewrite = true;
                        }
                    }
                }

                // all metadata with single quotes
                const char *METADATA_ATTRIBUTES_2[] = {
                    VENTANA_BASENAME_ATT_2,  VENTANA_FILENAME_ATT_2,  VENTANA_UNITNUMBER_ATT_2, VENTANA_USERNAME_ATT_2,
                    VENTANA_BUILDDATE_ATT_2, VENTANA_BARCODE1D_ATT_2, VENTANA_BARCODE2D_ATT_2};

                for (size_t k = 0; k < sizeof(METADATA_ATTRIBUTES_2) / sizeof(METADATA_ATTRIBUTES_2[0]); k++) {
                    if (contains(result, METADATA_ATTRIBUTES_2[k])) {
                        char *new_result = anonymize_xmp_attribute_if_exists(result, METADATA_ATTRIBUTES_2[k], "\'");
                        if (new_result != NULL) {
                            strcpy(result, new_result);
                            free(new_result);
                            rewrite = true;
                        }
                    }
                }

                // alters XML data of XMP tag
                if (rewrite) {
                    file_seek(fp, entry.offset, SEEK_SET);
                    if (!file_write(result, entry_size, entry.count, fp)) {
                        fprintf(stderr, "Error: Changing XML Data in XMP Tag failed.\n");
                        free(buffer);
                        return -1;
                    }
                }
                free(buffer);
            }

            // remove value in DATE_TIME tag
            if (entry.tag == TIFFTAG_DATETIME) {
                file_seek(fp, entry.offset, SEEK_SET);
                int32_t entry_size = get_size_of_value(entry.type, &entry.count);
                char *buffer = malloc(entry_size * entry.count);

                if (file_read(buffer, entry.count, entry_size, fp) != 1) {
                    fprintf(stderr, "Error: Could not read DATE_TIME Tag.\n");
                    free(buffer);
                    return -1;
                }

                char *replacement = create_replacement_string(' ', strlen(buffer));
                char *new_buffer = replace_str(buffer, buffer, replacement);
                file_seek(fp, entry.offset, SEEK_SET);
                if (!file_write(new_buffer, entry_size, entry.count, fp)) {
                    fprintf(stderr, "Error: Changing data in DATE_TIME Tag failed.\n");
                    free(replacement);
                    free(buffer);
                    free(new_buffer);
                    return -1;
                }
                free(replacement);
                free(buffer);
                free(new_buffer);
            }
        }
    }
    return 1;
}

// anonymizes ventana file
int32_t handle_ventana(const char **filename, const char *new_label_name, bool keep_macro_image, bool disable_unlinking,
                       bool do_inplace) {

    if (keep_macro_image) {
        fprintf(stderr, "Error: Cannot keep macro image in Ventana file.\n");
    }

    fprintf(stdout, "Anonymize Ventana WSI...\n");

    const char *ext = get_filename_ext(*filename);

    bool is_bif = strcmp(ext, BIF) == 0;

    if (!do_inplace) {
        *filename = duplicate_file(*filename, new_label_name, is_bif ? DOT_BIF : DOT_TIF);
    }

    file_handle *fp = file_open(*filename, "rb+");

    if (fp == NULL) {
        fprintf(stderr, "Error: Could not open tiff file.\n");
        return -1;
    }

    bool big_tiff = false;
    bool big_endian = false;
    int32_t result = check_file_header(fp, &big_endian, &big_tiff);

    if (result == -1) {
        file_close(fp);
        return -1;
    }

    struct tiff_file *file;
    file = read_tiff_file(fp, big_tiff, false, big_endian);

    if (file == NULL) {
        fprintf(stderr, "Error: Could not read tiff file.\n");
        file_close(fp);
        return -1;
    }

    int64_t label_dir = get_ventana_label_dir(fp, file);

    if (label_dir == -1) {
        fprintf(stderr, "Error: Could not find Image File Directory of Label image.\n");
        free_tiff_file(file);
        file_close(fp);
        return -1;
    }

    result = wipe_and_unlink_ventana_directory(fp, file, label_dir, big_endian, disable_unlinking);

    if (result == -1) {
        free_tiff_file(file);
        file_close(fp);
        return -1;
    }

    remove_metadata_in_ventana(fp, file);

    // clean up
    free((void *)(*filename));
    free_tiff_file(file);
    file_close(fp);
    return result;
}