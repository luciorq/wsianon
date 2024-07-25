#include "mirax-io.h"

static const char MRXS_EXT[] = "mrxs";
static const char DOT_MRXS_EXT[] = ".mrxs";
static const char SLIDEDAT[] = "Slidedat.ini";
static const char GENERAL[] = "GENERAL";
static const char SLIDE_NAME[] = "SLIDE_NAME";
static const char PROJECT_NAME[] = "PROJECT_NAME";
static const char SLIDE_ID[] = "SLIDE_ID";
static const char SLIDE_VERSION[] = "SLIDE_VERSION";
static const char SLIDE_CREATIONDATETIME[] = "SLIDE_CREATIONDATETIME";
static const char SLIDE_UTC_CREATIONDATETIME[] = "SLIDE_UTC_CREATIONDATETIME";
static const char PROFILENAME[] = "ProfileName=\"";
static const char NONHIERLAYER_0_SECTION[] = "NONHIERLAYER_0_SECTION";
static const char NONHIERLAYER_1_SECTION[] = "NONHIERLAYER_1_SECTION";
static const char SCANNER_HARDWARE_ID[] = "SCANNER_HARDWARE_ID";
static const char HIERARCHICAL[] = "HIERARCHICAL";
static const char INDEXFILE[] = "INDEXFILE";
static const char DATAFILE[] = "DATAFILE";
static const char FILE_COUNT[] = "FILE_COUNT";
static const char NONHIER_COUNT[] = "NONHIER_COUNT";
static const char NONHIER_X_COUNT[] = "NONHIER_%d_COUNT";
static const char NONHIER_X_NAME[] = "NONHIER_%d_NAME";
static const char NONHIER_X_VAL_X[] = "NONHIER_%d_VAL_%d";
static const char NONHIER_X_VAL_X_SECTION[] = "NONHIER_%d_VAL_%d_SECTION";
static const char NONHIER_X_VAL_X_IMAGENUMBER_X[] = "NONHIER_%d_VAL_%d_IMAGENUMBER_X";
static const char NONHIER_X_VAL_X_IMAGENUMBER_Y[] = "NONHIER_%d_VAL_%d_IMAGENUMBER_Y";
static const char _SECTION[] = "_SECTION";
static const char FILE_[] = "FILE_%d";

// define layer and level name in mirax file
static const char *SCAN_DATA_LAYER = "Scan data layer";
// slide label
static const char *SLIDE_BARCODE = "ScanDataLayer_SlideBarcode";
// macro image
static const char *SLIDE_THUMBNAIL = "ScanDataLayer_SlideThumbnail";
// whole slide image
static const char *SLIDE_WSI = "ScanDataLayer_WholeSlide";

struct metadata_attribute *get_attribute_mirax(struct ini_file *ini_file, const char *group_name,
                                               const char *metadata_key) {
    // iterate over groups in ini file
    for (int32_t i = 0; i < ini_file->group_count; i++) {
        struct ini_group *group = &ini_file->groups[i];
        // check for group
        if (strcmp(group->group_identifier, group_name) == 0) {
            for (int32_t j = 0; j < group->entry_count; j++) {
                struct ini_entry *entry = &group->entries[j];
                // if metadata_key was found
                if (strcmp(entry->key, metadata_key) == 0) {
                    struct metadata_attribute *single_attribute = malloc(sizeof(*single_attribute));
                    single_attribute->key = strdup(metadata_key);
                    single_attribute->value = strdup((*entry).value);
                    return single_attribute;
                }
            }
        }
    }
    return NULL;
}

struct metadata *get_metadata_mirax(const char *path, struct ini_file *ini_file, const char **data_filenames,
                                    int32_t num_of_datafiles) {
    // all groups
    static const char *GROUPS[] = {GENERAL, NONHIERLAYER_0_SECTION, NONHIERLAYER_1_SECTION};

    // all metadata
    static const char *METADATA_ATTRIBUTES[] = {
        SLIDE_NAME,          PROJECT_NAME, SLIDE_ID, SLIDE_CREATIONDATETIME, SLIDE_UTC_CREATIONDATETIME,
        SCANNER_HARDWARE_ID, PROFILENAME};

    // initialize metadata_attribute struct
    struct metadata_attribute **attributes =
        malloc(sizeof(**attributes) * sizeof(METADATA_ATTRIBUTES) / sizeof(METADATA_ATTRIBUTES[0]));
    int8_t metadata_id = 0;

    // checks for all metadata in Slidedat.ini
    for (size_t i = 0; i < sizeof(GROUPS) / sizeof(GROUPS[0]); i++) {
        for (size_t j = 0; j < sizeof(METADATA_ATTRIBUTES) / sizeof(METADATA_ATTRIBUTES[0]); j++) {
            struct metadata_attribute *single_attribute =
                get_attribute_mirax(ini_file, GROUPS[i], METADATA_ATTRIBUTES[j]);
            if (single_attribute != NULL) {
                attributes[metadata_id++] = single_attribute;
            }
        }
    }

    // iterate through every data file
    bool found_profilename = false;
    for (int32_t i = 0; i < num_of_datafiles && !found_profilename; i++) {
        // open data_file[i]
        const char *datadat_filename = concat_path_filename(path, data_filenames[i]);
        file_handle *fp = file_open(datadat_filename, "rb+");

        // if file does not exist terminate loop
        if (fp == NULL) {
            free((void *)datadat_filename);
            break;
        }

        // malloc buffer as big as slide version and slide id
        char *buffer = (char *)malloc(MRXS_MAX_SIZE_DATA_DAT);

        // read file
        if (file_read(buffer, MRXS_MAX_SIZE_DATA_DAT, 1, fp) != 1) {
            // check for ProfileName
            if (contains(buffer, PROFILENAME)) {
                // removes '=' from key and saves it with value in struct
                const char *value = get_string_between_delimiters(buffer, PROFILENAME, "\"");
                struct metadata_attribute *single_attribute = malloc(sizeof(*single_attribute));
                single_attribute->key = strdup(PROFILENAME);
                single_attribute->key[strlen(single_attribute->key) - 2] = '\0';
                single_attribute->value = strdup(value);
                if (single_attribute != NULL) {
                    attributes[metadata_id++] = single_attribute;
                }
                found_profilename = true;
            }
        }
        // cleanup
        free((void *)datadat_filename);
        free(buffer);
        file_close(fp);
    }

    // add all found metadata
    struct metadata *metadata_attributes = malloc(sizeof(*metadata_attributes));
    metadata_attributes->attributes = attributes;
    metadata_attributes->length = metadata_id;
    return metadata_attributes;
}

// free slidedat_ini with all groups and its entries
void free_slidedata_ini_file(struct ini_file *ini) {
    for (int32_t i = 0; i < ini->group_count; i++) {
        free((void *)(&ini->groups[i])->group_identifier);
    }
    free(ini);
}

struct wsi_data *get_wsi_data_mirax(const char *filename) {
    // gets file extension
    const char *ext = get_filename_ext(filename);

    // check for valid file extension
    if (strcmp(ext, MRXS_EXT) != 0) {
        return NULL;
    }

    // open ini file
    char *path = strndup(filename, strlen(filename) - strlen(DOT_MRXS_EXT));
    struct ini_file *ini = read_slidedat_ini_file(path, SLIDEDAT);

    // check if ini file was successfully read
    if (ini == NULL) {
        return NULL;
    }

    // get number of files
    const char *file_count = get_value_from_ini_file(ini, DATAFILE, FILE_COUNT);
    if (file_count == NULL) {
        fprintf(stderr, "Error: No index file specified.\n");
        return NULL;
    }

    // cast file count
    int32_t f_count;
    sscanf(file_count, "%d", &f_count);

    // get data.dat filenames for metadata
    const char **data_filenames = (const char **)malloc((f_count + 1) * sizeof(char *));
    for (int32_t i = 0; i < f_count; i++) {
        const char *temp_datafile_key = concat_wildcard_string_int32(FILE_, i);
        const char *temp_datafile_name = get_value_from_ini_file(ini, DATAFILE, temp_datafile_key);
        data_filenames[i] = temp_datafile_name;
    }

    // gets all metadata in Slidedat.ini
    struct metadata *metadata_attributes = get_metadata_mirax(path, ini, data_filenames, f_count);

    // is MIRAX
    struct wsi_data *wsi_data = malloc(sizeof(*wsi_data));
    wsi_data->format = MIRAX;
    wsi_data->filename = filename;
    wsi_data->metadata_attributes = metadata_attributes;

    // cleanup
    free(path);
    free_slidedata_ini_file(ini);
    free(data_filenames);
    return wsi_data;
}

// retrieve the file structure of the mirax file from the
// Slidedat.ini file
struct mirax_file *get_mirax_file_structure(struct ini_file *ini, int32_t l_count) {
    // initialize mirax file and array of associated layers
    struct mirax_file *mirax_file = (struct mirax_file *)malloc(sizeof(struct mirax_file));

    struct mirax_layer **layers = (struct mirax_layer **)malloc(l_count * sizeof(struct mirax_layer));

    int32_t records = 0;
    for (int32_t layer_id = 0; layer_id < l_count; layer_id++) {
        // initialize and populate new layer
        struct mirax_layer *layer = (struct mirax_layer *)malloc(sizeof(struct mirax_layer));

        const char *nonhier_level_count_key = concat_wildcard_string_int32(NONHIER_X_COUNT, layer_id);

        const char *nonhier_level_count = get_value_from_ini_file(ini, HIERARCHICAL, nonhier_level_count_key);

        int32_t nhl_count;
        sscanf(nonhier_level_count, "%d", &nhl_count);
        layer->level_count = nhl_count;

        const char *layer_name_key = concat_wildcard_string_int32(NONHIER_X_NAME, layer_id);

        layer->layer_name = get_value_from_ini_file(ini, HIERARCHICAL, layer_name_key);

        // initialize and allocate new array of sub lavels of array
        struct mirax_level **levels = (struct mirax_level **)malloc(nhl_count * sizeof(struct mirax_level));

        for (int32_t level_id = 0; level_id < nhl_count; level_id++) {
            // initialize and populate sublevels
            // printf("layer %i, level %i\n", layer_id, level_id);
            struct mirax_level *level = (struct mirax_level *)malloc(sizeof(struct mirax_level));

            level->id = level_id;
            level->layer_id = layer_id;
            level->record = layer_id + level_id;
            const char *key_prefix = concat_wildcard_string_m_int32(NONHIER_X_VAL_X, layer_id, level_id);
            level->key_prefix = key_prefix;
            level->name = get_value_from_ini_file(ini, HIERARCHICAL, level->key_prefix);
            int32_t length = strlen(NONHIER_X_VAL_X) + number_of_digits(layer_id) + number_of_digits(level_id) + 1;
            char *section_key = (char *)malloc(length + sizeof(_SECTION) + 2);
            strcpy(section_key, level->key_prefix);
            strcat(section_key, _SECTION);
            level->section_key = section_key;
            level->section = get_value_from_ini_file(ini, HIERARCHICAL, level->section_key);
            levels[level_id] = level;
            records++;
        }
        // add level array to layer
        layer->levels = levels;
        layers[layer_id] = layer;
    }

    // add layer arrays to mirax file
    mirax_file->all_records_count = records;
    mirax_file->count_layers = l_count;
    mirax_file->layers = layers;
    return mirax_file;
}

// retrive mirax level from file structure by
// layer and level name
struct mirax_level *get_level_by_name(struct mirax_layer **layers, const char *layer_name, const char *level_name) {
    int32_t length = sizeof(**layers) / sizeof(layers[0]);
    for (int32_t i = 0; i < length; i++) {
        struct mirax_layer *layer = layers[i];
        // printf("layer name %s\n", layer->layer_name);
        if (strcmp(layer->layer_name, layer_name) == 0) {
            for (int32_t j = 0; j < layer->level_count; j++) {
                struct mirax_level *level = layer->levels[j];
                // printf("level name %s\n", level->name);
                if (strcmp(level->name, level_name) == 0) {
                    return level;
                }
            }
        }
    }
    return NULL;
}

// read file number, position and size frrom index dat
int32_t *read_data_location(const char *filename, int32_t record, int32_t **position, int32_t **size) {
    file_handle *fp = file_open(filename, "rb+");

    if (fp == NULL) {
        fprintf(stderr, "Error: Could not open file stream.\n");
        return NULL;
    }

    if (file_seek(fp, MRXS_ROOT_OFFSET_NONHIER, SEEK_SET)) {
        fprintf(stderr, "Error: Can not seek to mrxs offset in Index.dat.\n");
        file_close(fp);
        return NULL;
    }

    int32_t *table_base = read_int32(fp);
    file_seek(fp, (*table_base + (record * 4)), SEEK_SET);

    int32_t *list_header = read_int32(fp);
    file_seek(fp, *list_header, SEEK_SET);
    // assert we found the list head
    if (!assert_value(fp, 0)) {
        file_close(fp);
        return NULL;
    }

    int32_t *page = read_int32(fp);
    file_seek(fp, *page, SEEK_SET);
    // assert we found the page
    if (!assert_value(fp, 1)) {
        file_close(fp);
        return NULL;
    }

    // move on 3*4bytes
    read_int32(fp);
    if (!assert_value(fp, 0)) {
        file_close(fp);
        return NULL;
    }
    if (!assert_value(fp, 0)) {
        file_close(fp);
        return NULL;
    }

    // read in the file number, offset and
    // length of the image strip
    *position = read_int32(fp);
    *size = read_int32(fp);
    int32_t *fileno = read_int32(fp);

    file_close(fp);
    return fileno;
}

// wipe the image data for filename, offset and length
int32_t wipe_level_data(const char *filename, int32_t **offset, int32_t **length, const char *prefix,
                        const char *suffix) {
    file_handle *fp = file_open(filename, "rb+");

    if (fp == NULL) {
        fprintf(stderr, "Error: Could not open label file.\n");
        return -1;
    }

    if (file_seek(fp, **offset, SEEK_SET)) {
        fprintf(stderr, "Error: Failed to seek to offset.\n");
        file_close(fp);
        return -1;
    }

    char *buffer = (char *)malloc(sizeof(prefix));
    if (file_read(buffer, sizeof(prefix), 1, fp) != 1) {
        fprintf(stderr, "Error: Could not read strip prefix.\n");
        file_close(fp);
        free(buffer);
        return -1;
    }

    // check for jpeg prefix start of file (soi)
    if (buffer[0] != prefix[0] || buffer[1] != prefix[1]) {
        fprintf(stderr, "Error: Unexpected data in file.\n");
        file_close(fp);
        free(buffer);
        return -1;
    }

    // write empty jpeg image to file
    char *empty_buffer = create_pre_suffixed_char_array('0', **length, prefix, suffix);
    file_seek(fp, **offset, SEEK_SET);
    file_write(empty_buffer, **length, 1, fp);

    free(empty_buffer);
    free(buffer);
    file_close(fp);
    return 0;
}

// remove label level
int32_t delete_level(const char *path, const char *index_file, const char **data_files, struct mirax_layer **layers,
                     const char *layer_name, const char *level_name) {
    struct mirax_level *level_to_delete = get_level_by_name(layers, layer_name, level_name);

    if (level_to_delete == NULL) {
        fprintf(stderr, "Warning: Could not find expected level with name %s. Unable to delete non-existing level.\n",
                level_name);
        return 0;
    }

    const char *index_file_path = concat_path_filename(path, index_file);
    int32_t *position, *size;
    int32_t *fileno = read_data_location(index_file_path, level_to_delete->record, &position, &size);

    if (fileno == NULL) {
        return -1;
    }

    const char *filename = concat_path_filename(path, data_files[*fileno]);

    return wipe_level_data(filename, &position, &size, JPEG_SOI, JPEG_EOI);
}

// delete label record from index file
int32_t delete_record_from_index_file(const char *filename, int32_t record, int32_t all_records) {
    file_handle *fp = file_open(filename, "rb+");
    int32_t to_move = all_records - record - 1;

    if (to_move == 0) {
        file_close(fp);
        return 0;
    }

    file_seek(fp, MRXS_ROOT_OFFSET_NONHIER, SEEK_SET);
    int32_t *table_base = read_int32(fp);
    int32_t offset = (*table_base) + (record + 1) * 4;
    file_seek(fp, offset, SEEK_SET);

    char buffer[to_move * 4];
    if (file_read(buffer, (to_move * 4), 1, fp) != 1) {
        fprintf(stderr, "Error: Unexpected length of buffer.\n");
        file_close(fp);
        return -1;
    }

    // we return to record position and overwrite
    // the file with the tail data
    file_seek(fp, (*table_base + (record * 4)), SEEK_SET);
    file_write(&buffer, (to_move * 4), 1, fp);

    file_close(fp);
    return 0;
}

// duplicate and rename the mirax file file.mrxs and
// the associated folder with the image data
// return new path name of image data folder
// filename will be modified to new filename
const char *duplicate_mirax_filedata(const char *filename, const char *new_label_name, const char *file_extension) {
    // retrive filename from whole file path
    const char *_filename = get_filename_from_path(filename);

    if (_filename == NULL) {
        fprintf(stderr, "Error: Could not retrieve filename from filepath.\n");
        return NULL;
    }

    // get the directory path
    int32_t diff = strlen(filename) - strlen(_filename);
    char path[diff + 1];
    memcpy(path, &filename[0], diff);
    path[diff] = '\0';

    int32_t diff2 = strlen(_filename) - strlen(file_extension);
    char old_filename_wo_ext[diff2 + 1];
    memcpy(old_filename_wo_ext, &_filename[0], diff2);
    old_filename_wo_ext[diff2] = '\0';

    // now we can concat the new filename
    if (new_label_name == NULL) {
        // if no label is given, we give the file a generic name
        char dummy_prefix[] = "ANONYMIZED_";
        char *temp_label_name = (char *)malloc(strlen(dummy_prefix) + strlen(old_filename_wo_ext) + 1);
        strcpy(temp_label_name, dummy_prefix);
        strcat(temp_label_name, old_filename_wo_ext);
        // concat string
        new_label_name = temp_label_name;
    }
    const char *new_filename = concat_path_filename_ext(path, new_label_name, file_extension);

    if (file_exists(new_filename)) {
        return NULL;
    }

    int32_t copy_result = copy_file_v2(filename, new_filename);
    // we copy the file in our current directory
    if (copy_result != 0) {
        return NULL;
    }

    filename = new_filename;

    // copy mirax directory
    char *new_path = (char *)malloc(strlen(path) + strlen(new_label_name) + 1);
    strcpy(new_path, path);
    strcat(new_path, new_label_name);

    char *old_path = (char *)malloc(strlen(path) + strlen(old_filename_wo_ext) + 1);
    strcpy(old_path, path);
    strcat(old_path, old_filename_wo_ext);

    int32_t result = copy_directory(old_path, new_path);
    free(old_path);

    if (result != -1) {
        return new_path;
    } else {
        return NULL;
    }
}

// remove a level entry by id from mirax layer structure
struct mirax_layer *delete_level_by_id(struct mirax_layer *layer, int32_t level_id) {
    struct mirax_layer *temp = (struct mirax_layer *)malloc(sizeof(struct mirax_layer));
    temp->layer_name = layer->layer_name;
    temp->level_count = layer->level_count - 1;

    // new array with size one less than old array
    struct mirax_level **temp_levels = (struct mirax_level **)malloc(temp->level_count * sizeof(struct mirax_level));

    // copy all elements before the index
    if (level_id != 0) {
        memcpy(temp_levels, layer->levels, level_id * sizeof(struct mirax_level));
    }

    // copy all elements after the index
    if (level_id != (layer->level_count - 1)) {
        memcpy(temp_levels + level_id, layer->levels + level_id + 1,
               (temp->level_count - level_id) * sizeof(struct mirax_level));
    }

    temp->levels = temp_levels;

    free(layer);
    return temp;
}

void rename_mirax_file_levels(struct mirax_file *mirax_file, int32_t layer, struct mirax_level *current_level,
                              struct mirax_level *next_level) {
    char *name_copy = (char *)malloc(strlen(next_level->name) + 1);
    strcpy(name_copy, next_level->name);
    mirax_file->layers[layer]->levels[current_level->id]->name = name_copy;
    char *section_copy = (char *)malloc(strlen(next_level->name) + 1);
    strcpy(section_copy, next_level->name);
    mirax_file->layers[layer]->levels[current_level->id]->section = section_copy;
}

void delete_last_entry_from_ini_file(struct ini_file *ini, struct mirax_file *mirax_file, int32_t layer_id) {
    // change count in hierarchy layer
    decrement_value_for_group_and_key(ini, HIERARCHICAL, concat_wildcard_string_int32(NONHIER_X_COUNT, layer_id));

    int32_t id_to_delete = mirax_file->layers[layer_id]->level_count - 1;

    // we need to delete all old unused entries now
    const char *nonhier_plain = concat_wildcard_string_m_int32(NONHIER_X_VAL_X, layer_id, id_to_delete);
    const char *nonhier_section = concat_wildcard_string_m_int32(NONHIER_X_VAL_X_SECTION, layer_id, id_to_delete);
    const char *nonhier_imgx = concat_wildcard_string_m_int32(NONHIER_X_VAL_X_IMAGENUMBER_X, layer_id, id_to_delete);
    const char *nonhier_imgy = concat_wildcard_string_m_int32(NONHIER_X_VAL_X_IMAGENUMBER_Y, layer_id, id_to_delete);
    remove_entry_for_group_and_key(ini, HIERARCHICAL, nonhier_plain);
    remove_entry_for_group_and_key(ini, HIERARCHICAL, nonhier_section);
    remove_entry_for_group_and_key(ini, HIERARCHICAL, nonhier_imgx);
    remove_entry_for_group_and_key(ini, HIERARCHICAL, nonhier_imgy);
    mirax_file->layers[layer_id]->level_count--;
}

// unlink the levels and restructure slidedat.ini
void unlink_level(struct ini_file *ini, struct mirax_level *level_to_delete, struct mirax_file *mirax_file) {

    int32_t layer_id = level_to_delete->layer_id;

    for (int32_t i = 0; i < mirax_file->layers[layer_id]->level_count; i++) {
        if (strcmp(mirax_file->layers[layer_id]->levels[i]->name, level_to_delete->name) == 0) {
            // printf("%d\n", i);
            restructure_levels_in_file(ini, i, layer_id, mirax_file);
        }
    }

    delete_last_entry_from_ini_file(ini, mirax_file, layer_id);
}

// change the value for slide id in index.dat to the same as in slidedat
int32_t replace_slide_id_in_indexdat(const char *path, const char *filename, const char *value, const char *replacement,
                                     int32_t size) {

    // concat index.dat filename
    const char *indexdat_filename = concat_path_filename(path, filename);

    file_handle *fp = file_open(indexdat_filename, "rb+");

    if (fp == NULL) {
        fprintf(stderr, "Error: Could not open index.dat file.\n");
        return -1;
    }

    file_seek(fp, 0, SEEK_SET);

    // malloc buffer as big as slide version and slide id
    char *buffer = (char *)malloc(size);

    // overwrite value for slide id in index.dat
    if (file_gets(buffer, size, fp) != NULL) {
        if (contains(buffer, value)) {
            buffer = replace_str(buffer, value, replacement);
            file_seek(fp, 0, SEEK_SET);
            if (file_write(buffer, size - 1, 1, fp) != 1) {
                fprintf(stderr, "Error: Could not overwrite slide id in index.dat.\n");
                return -1;
            }
        }
    }

    free(buffer);
    file_close(fp);

    return 1;
}

// change the value for slide id in all dat files to the same as in slidedat
int32_t replace_slide_id_in_datfiles(const char *path, const char **data_files, int32_t length, const char *value,
                                     const char *replacement, int32_t size) {

    // iterate through every data file
    for (int32_t i = 0; i < length; i++) {

        const char *datadat_filename = concat_path_filename(path, data_files[i]);

        file_handle *fp = file_open(datadat_filename, "rb+");

        // if file does not exist terminate loop
        if (fp == NULL) {
            break;
        }

        file_seek(fp, 0, SEEK_SET);

        // malloc buffer as big as slide version and slide id
        char *buffer = (char *)malloc(size);

        // overwrite value for slide id in data.dat files
        if (file_gets(buffer, size, fp) != NULL) {
            if (contains(buffer, value)) {
                buffer = replace_str(buffer, value, replacement);
                file_seek(fp, 0, SEEK_SET);
                if (file_write(buffer, size - 1, 1, fp) != 1) {
                    fprintf(stderr, "Error: Could not overwrite slide id in data.dat.\n");
                }
            }
        }

        free(buffer);
        file_close(fp);
    }
    return 1;
}

void remove_metadata_in_data_dat(const char *path, const char **data_files, int32_t length) {

    // iterate through every data file
    for (int32_t i = 0; i < length; i++) {

        const char *datadat_filename = concat_path_filename(path, data_files[i]);

        file_handle *fp = file_open(datadat_filename, "rb+");

        // if file does not exist terminate loop
        if (fp == NULL) {
            break;
        }

        // malloc buffer as big as slide version and slide id
        char *buffer = (char *)malloc(MRXS_MAX_SIZE_DATA_DAT);

        // read file
        if (file_read(buffer, MRXS_MAX_SIZE_DATA_DAT, 1, fp) != 1) {
            // check for ProfileName
            if (contains(buffer, PROFILENAME)) {
                const char *value = get_string_between_delimiters(buffer, PROFILENAME, "\"");
                char *replacement = create_replacement_string('X', strlen(value));
                buffer = replace_str(buffer, value, replacement);
            }
            // overwrite value in data.dat file
            file_seek(fp, 0, SEEK_SET);
            if (file_write(buffer, MRXS_MAX_SIZE_DATA_DAT, 1, fp) != 1) {
                fprintf(stderr, "Error: Could not overwrite value in %s.\n", data_files[i]);
            }
        }

        free(buffer);
        file_close(fp);
    }
}

int32_t wipe_data_in_index_file(const char *path, const char *index_filename, struct mirax_level *level_to_delete,
                                struct mirax_file *mirax_file) {
    int32_t result = -1;
    // delete record in index dat
    const char *full_index_filename = concat_path_filename(path, index_filename);
    result = delete_record_from_index_file(full_index_filename, level_to_delete->record, mirax_file->all_records_count);
    return result;
}

int32_t wipe_delete_unlink(const char *path, struct ini_file *ini, const char *index_filename,
                           struct mirax_file *mirax_file, const char *layer, const char *level_to_delete) {

    struct mirax_level *level = get_level_by_name(mirax_file->layers, layer, level_to_delete);

    if (level != NULL) {
        if (wipe_data_in_index_file(path, index_filename, level, mirax_file) != 0) {
            fprintf(stderr, "Error: Failed to wipe data in Index.dat.\n");
            return -1;
        }

        if (delete_group_form_ini_file(ini, level->section) != 0) {
            fprintf(stderr, "Error: Failed to delete group in Slidedat.ini.\n");
            return -1;
        }

        unlink_level(ini, level, mirax_file);
    }
    return 1;
}

char *strndup(const char *s1, size_t n) {
    char *copy = (char *)malloc(n + 1);
    memcpy(copy, s1, n);
    copy[n] = '\0';
    return copy;
}

int32_t handle_mirax(const char **filename, const char *new_label_name, bool keep_macro_image, bool disable_unlinking,
                     bool do_inplace) {
    fprintf(stdout, "Anonymize Mirax WSI...\n");

    const char *path = strndup(*filename, strlen(*filename) - strlen(DOT_MRXS_EXT));

    if (!do_inplace) {
        path = duplicate_mirax_filedata(*filename, new_label_name, DOT_MRXS_EXT);

        if (path == NULL || filename == NULL) {
            fprintf(stderr, "Error: File with stated filename already exists. Remove file or set "
                            "label name.\n");
            return -1;
        }
    }

    struct ini_file *ini = read_slidedat_ini_file(path, SLIDEDAT);
    const char *index_filename = get_value_from_ini_file(ini, HIERARCHICAL, INDEXFILE);
    const char *file_count = get_value_from_ini_file(ini, DATAFILE, FILE_COUNT);
    const char *layer_count = get_value_from_ini_file(ini, HIERARCHICAL, NONHIER_COUNT);

    if (index_filename == NULL || file_count == NULL || layer_count == NULL) {
        fprintf(stderr, "Error: No index file specified.\n");
        return -1;
    }

    // cast file count
    int32_t f_count;
    int32_t l_count;
    sscanf(file_count, "%d", &f_count);
    sscanf(layer_count, "%d", &l_count);

    // get .dat filenames
    const char **data_filenames = (const char **)malloc((f_count + 1) * sizeof(char *));
    for (int32_t i = 0; i < f_count; i++) {
        const char *temp_datafile_key = concat_wildcard_string_int32(FILE_, i);
        const char *temp_datafile_name = get_value_from_ini_file(ini, DATAFILE, temp_datafile_key);
        data_filenames[i] = temp_datafile_name;
    }

    struct mirax_file *mirax_file = get_mirax_file_structure(ini, l_count);

    // wipe the image data in the data file
    // slide label
    int32_t result =
        delete_level(path, index_filename, data_filenames, mirax_file->layers, SCAN_DATA_LAYER, SLIDE_BARCODE);

    // check for result
    if (result == -1) {
        fprintf(stderr, "Error: Could not wipe slide label.\n");
        return -1;
    }

    // delete macro image
    if (!keep_macro_image) {
        result =
            delete_level(path, index_filename, data_filenames, mirax_file->layers, SCAN_DATA_LAYER, SLIDE_THUMBNAIL);

        // check for result
        if (result == -1) {
            fprintf(stderr, "Error: Could not wipe macro image.\n");
            return -1;
        }
    }

    // delete whole slide image
    result = delete_level(path, index_filename, data_filenames, mirax_file->layers, SCAN_DATA_LAYER, SLIDE_WSI);

    // check for result
    if (result == -1) {
        fprintf(stderr, "Error: Could not wipe whole slide image.\n");
        return -1;
    }

    // unlink directory
    if (!disable_unlinking) {
        // THIS IS A QUICKFIX!
        // The order of label/macro images etc. actually matters in the file structure of the
        // Slidedat.ini
        // TODO: investigate why the barcode/wsi level is not removed when order in Slidedat.ini is
        // different
        struct mirax_level *barcode_layer = get_level_by_name(mirax_file->layers, SCAN_DATA_LAYER, SLIDE_BARCODE);
        int32_t barcode_group_index = -1;
        if (barcode_layer != NULL) {
            barcode_group_index = get_group_index_of_ini_file(ini, barcode_layer->section);
        }
        struct mirax_level *wsi_layer = get_level_by_name(mirax_file->layers, SCAN_DATA_LAYER, SLIDE_WSI);
        int32_t wsi_group_index = -1;
        if (wsi_layer != NULL) {
            wsi_group_index = get_group_index_of_ini_file(ini, wsi_layer->section);
        }

        if (barcode_group_index != -1 || wsi_group_index != -1) {
            if (barcode_group_index < wsi_group_index) {
                wipe_delete_unlink(path, ini, index_filename, mirax_file, SCAN_DATA_LAYER, SLIDE_WSI);
                wipe_delete_unlink(path, ini, index_filename, mirax_file, SCAN_DATA_LAYER, SLIDE_BARCODE);
            } else {
                wipe_delete_unlink(path, ini, index_filename, mirax_file, SCAN_DATA_LAYER, SLIDE_BARCODE);
                wipe_delete_unlink(path, ini, index_filename, mirax_file, SCAN_DATA_LAYER, SLIDE_WSI);
            }
        }

        // unlink macro image
        if (!keep_macro_image) {
            wipe_delete_unlink(path, ini, index_filename, mirax_file, SCAN_DATA_LAYER, SLIDE_THUMBNAIL);
        }
    }

    // remove metadata in slidedata ini
    printf("Removing metadata in Slidedat.ini...\n");
    anonymize_value_for_group_and_key(ini, GENERAL, SLIDE_NAME, 'X');
    anonymize_value_for_group_and_key(ini, GENERAL, PROJECT_NAME, 'X');
    anonymize_value_for_group_and_key(ini, GENERAL, SLIDE_UTC_CREATIONDATETIME, 'X');
    anonymize_value_for_group_and_key(ini, NONHIERLAYER_0_SECTION, SCANNER_HARDWARE_ID, 'X');
    anonymize_value_for_group_and_key(ini, NONHIERLAYER_1_SECTION, SCANNER_HARDWARE_ID, 'X');

    // set predefined value for SLIDE_CREATIONDATETIME
    anonymize_value_for_group_and_key_with_given_string(ini, GENERAL, SLIDE_CREATIONDATETIME,
                                                        concat_str(concat_str(MIN_DATE, " "), MIN_TIME));

    // remove metadata in data dat files
    remove_metadata_in_data_dat(path, data_filenames, f_count);

    // remove slide id in slidedat, indexfile and data files
    const char *slide_id = get_value_from_ini_file(ini, GENERAL, SLIDE_ID);
    const char *replacement = create_random_slide_id(ini, GENERAL, SLIDE_ID);
    const char *slide_version = get_value_from_ini_file(ini, GENERAL, SLIDE_VERSION);
    int32_t size = strlen(slide_version) + strlen(slide_id) + 1;
    replace_slide_id_in_indexdat(path, index_filename, slide_id, replacement, size);
    replace_slide_id_in_datfiles(path, data_filenames, f_count, slide_id, replacement, size);

    if (write_ini_file(ini, path, SLIDEDAT) == -1) {
        return -1;
    }

    free(data_filenames);
    free(mirax_file);
    free(ini);

    if (!do_inplace) {
        // override with new filename
        *filename = path;
    }

    return result;
}