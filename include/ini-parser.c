#include "ini-parser.h"

// retrieve an entry value from the ini file by
// given group and entry key
const char *get_value_from_ini_file(struct ini_file *ini_file, const char *group_name, const char *key) {
    struct ini_group *ini_group = find_group(ini_file, group_name);
    if (ini_group != NULL) {
        struct ini_entry *entry = find_entry(ini_group, key);
        if (entry != NULL) {
            return entry->value;
        }
    }
    return NULL;
}

// get the amount of ini groups in the ini
int32_t get_groups_count(file_handle *fp) {
    int32_t count_groups = 0;
    // we only need the first two chars to determine
    // if line is a group tag
    char *buffer = (char *)malloc(2);
    while (file_gets(buffer, 2, fp) != NULL) {
        if (strstr(buffer, "[") != NULL) {
            count_groups++;
        }
    }
    free(buffer);
    return count_groups;
}

struct ini_file *read_slidedat_ini_file(const char *path, const char *ini_filename) {
    // concat slidedat filename
    const char *slidedat_filename = concat_path_filename(path, ini_filename);

    file_handle *fp = file_open(slidedat_filename, "r");

    if (fp == NULL) {
        fprintf(stderr, "Error: Could not read ini file.\n");
        return NULL;
    }

    // determine the amount of ini groups
    int32_t count_groups = get_groups_count(fp);
    // printf("Count groups[%i]\n", count_groups);
    // we return to the file start
    file_seek(fp, 0, SEEK_SET);

    // initialize line buffer
    char *buffer = (char *)malloc(MAX_CHAR_IN_LINE);

    // initialize ini groups
    struct ini_group *groups = (struct ini_group *)malloc(count_groups * sizeof(struct ini_group));
    int32_t line = 0, group_count = 0;

    while (file_gets(buffer, MAX_CHAR_IN_LINE, fp) != NULL) {
        if (strstr(buffer, "[") != NULL && strstr(buffer, "]") != NULL) {
            // create new group and populate
            struct ini_group *temp_group = (struct ini_group *)malloc(sizeof(struct ini_group));
            const char *group_id = get_string_between_delimiters(buffer, "[", "]");
            temp_group->group_identifier = strdup(group_id);
            temp_group->start_line = line;
            // add group to array
            groups[group_count] = *temp_group;
            group_count++;
        }
        line++;
    }
    // set last group start line to end of file
    groups[count_groups].start_line = line;

    // populate the ini groups with associated entries
    for (int32_t i = 0; i < count_groups; i++) {
        struct ini_group current_group = groups[i];
        // printf("-group: [%s]\n", current_group.group_identifier);
        struct ini_group next_group = groups[i + 1];

        int32_t entry_size = next_group.start_line - current_group.start_line;
        struct ini_entry *entries = (struct ini_entry *)malloc(entry_size * sizeof(struct ini_entry));

        // each iteration we return to the file start
        file_seek(fp, 0, SEEK_SET);

        // read lines related to the current group
        int32_t line_count = 0, entry_count = 0;
        while (file_gets(buffer, MAX_CHAR_IN_LINE, fp) != NULL) {
            line_count++;
            // skip useless lines
            if (line_count <= current_group.start_line + 1) {
                continue;
            } else if (count_groups != i + 1 && line_count > next_group.start_line) {
                break;
            }

            // parse entry
            struct ini_entry *temp_entry = (struct ini_entry *)malloc(sizeof(struct ini_entry));
            if (!contains(buffer, "=")) {
                continue;
            }
            char **splitted_entry = str_split(buffer, '=');

            // remove whitspace
            char *key = splitted_entry[0];
            remove_leading_spaces(key);
            temp_entry->key = key;
            // remove tab
            char *value = str_split(splitted_entry[1], '\r')[0];
            remove_leading_spaces(value);
            temp_entry->value = value;
            entries[entry_count] = *temp_entry;
            entry_count++;

            // printf("--entry: [%s / %s]\n", key, value);
        }
        groups[i].entries = entries;
        groups[i].entry_count = entry_count;
    }

    // TODO: Investigate why closing of some files cause a `corrupted size vs. prev_size` error
    // here? Anyway, this shouldn't make any troubles here, because the file is released after
    // termination anyway
    // file_close(fp);
    free(buffer);

    struct ini_file *ini_file = (struct ini_file *)malloc(sizeof(struct ini_file));
    ini_file->group_count = group_count;
    ini_file->groups = groups;

    return ini_file;
}

struct ini_group *remove_ini_group_from_array(struct ini_group *groups, int32_t size_of_array,
                                              int32_t index_to_remove) {
    // new array with size one less than old array
    struct ini_group *temp = (struct ini_group *)malloc((size_of_array - 1) * sizeof(struct ini_group));

    // copy all elements before the index
    if (index_to_remove != 0) {
        memcpy(temp, groups, index_to_remove * sizeof(struct ini_group));
    }

    // copy all elements after the index
    if (index_to_remove != (size_of_array - 1)) {
        memcpy(temp + index_to_remove, groups + index_to_remove + 1,
               (size_of_array - index_to_remove - 1) * sizeof(struct ini_group));
    }

    free(groups);
    return temp;
}

int32_t delete_group_form_ini_file(struct ini_file *ini_file, const char *group_name) {
    // get the group index
    int32_t group_index = get_group_index_of_ini_file(ini_file, group_name);
    if (group_index == -1) {
        return -1;
    }

    // remove group from array and assign new group pointer to group
    struct ini_group *new_groups = remove_ini_group_from_array(ini_file->groups, ini_file->group_count, group_index);
    ini_file->groups = new_groups;
    ini_file->group_count--;

    return 0;
}

int32_t get_group_index_of_ini_file(struct ini_file *ini_file, const char *group_name) {
    int32_t group_index = -1;
    for (int32_t i = 0; i < ini_file->group_count; i++) {
        struct ini_group group = ini_file->groups[i];
        if (strcmp(group.group_identifier, group_name) == 0) {
            group_index = i;
            break;
        }
    }
    return group_index;
}

// modify structure of levels in mirax_file
void restructure_levels_in_file(struct ini_file *ini_file, int32_t level_pos_in_layer, int32_t layer_id,
                                struct mirax_file *mirax_file) {
    for (int32_t i = level_pos_in_layer; i < mirax_file->layers[layer_id]->level_count - 1; i++) {
        restructure_groups_in_file(ini_file, mirax_file->layers[layer_id]->levels[i],
                                   mirax_file->layers[layer_id]->levels[i + 1]);
        mirax_file->layers[layer_id]->levels[i]->name = mirax_file->layers[layer_id]->levels[i + 1]->name;
    }
}

// modify structure of groups in ini_file
void restructure_groups_in_file(struct ini_file *ini_file, struct mirax_level *current_level,
                                struct mirax_level *next_level) {
    struct ini_group *group = find_group(ini_file, "HIERARCHICAL");
    if (group != NULL) {
        for (int32_t j = 0; j < group->entry_count; j++) {

            struct ini_entry *entry = &group->entries[j];

            if (entry != NULL) {

                if (strcmp(entry->value, current_level->name) == 0 &&
                    strcmp(group->entries[j + MRXS_SLIDE_DAT_NONHIER_GROUP_OFFSET].value, next_level->name) == 0) {
                    group->entries[j].value = group->entries[j + 4].value; // change level name
                    group->entries[j + 1].value =
                        group->entries[j + MRXS_SLIDE_DAT_NONHIER_GROUP_OFFSET + 1].value; // change section
                }
            }
        }
    }
}

struct ini_group *find_group(struct ini_file *ini_file, const char *group_name) {
    for (int32_t i = 0; i < ini_file->group_count; i++) {
        struct ini_group *group = &ini_file->groups[i];
        if (strcmp(group->group_identifier, group_name) == 0) {
            return group;
        }
    }
    return NULL;
}

struct ini_entry *find_entry(struct ini_group *group, const char *key) {
    for (int32_t j = 0; j < group->entry_count; j++) {
        struct ini_entry *entry = &group->entries[j];
        if (strcmp(entry->key, key) == 0) {
            return entry;
        }
    }
    return NULL;
}

const char *anonymize_value_for_group_and_key(struct ini_file *ini_file, const char *group_name, const char *key,
                                              const char c) {
    struct ini_group *group = find_group(ini_file, group_name);
    if (group != NULL) {
        struct ini_entry *entry = find_entry(group, key);
        if (entry != NULL) {
            const char *value = create_replacement_string(c, strlen((*entry).value));
            (*entry).value = strdup(value);
            return value;
        }
    }
    return NULL;
}

const char *anonymize_value_for_group_and_key_with_given_string(struct ini_file *ini_file, const char *group_name,
                                                                const char *key, const char *value) {
    struct ini_group *group = find_group(ini_file, group_name);
    if (group != NULL) {
        struct ini_entry *entry = find_entry(group, key);
        if (entry != NULL) {
            (*entry).value = strdup(value);
            return value;
        }
    }
    return NULL;
}

const char *create_random_slide_id(struct ini_file *ini_file, const char *group_name, const char *key) {
    struct ini_group *group = find_group(ini_file, group_name);
    if (group != NULL) {
        struct ini_entry *entry = find_entry(group, key);
        if (entry != NULL) {
            const char *value = create_random_string(strlen((*entry).value));
            (*entry).value = strdup(value);
            return value;
        }
    }
    return NULL;
}

struct ini_entry *remove_ini_entry_from_array(struct ini_entry *entries, int32_t size_of_array,
                                              int32_t index_to_remove) {
    // new array with size one less than old array
    struct ini_entry *temp = (struct ini_entry *)malloc((size_of_array - 1) * sizeof(struct ini_entry));

    // copy all elements before the index
    if (index_to_remove != 0) {
        memcpy(temp, entries, index_to_remove * sizeof(struct ini_entry));
    }

    // copy all elements after the index
    if (index_to_remove != (size_of_array - 1))
        memcpy(temp + index_to_remove, entries + index_to_remove + 1,
               (size_of_array - index_to_remove - 1) * sizeof(struct ini_entry));

    free(entries);
    return temp;
}

void remove_entry_for_group_and_key(struct ini_file *ini_file, const char *group_name, const char *key) {
    int32_t c_group = -1, c_entry = -1;
    for (int32_t i = 0; i < ini_file->group_count; i++) {
        struct ini_group *group = &ini_file->groups[i];
        if (strcmp(group->group_identifier, group_name) == 0) {
            for (int32_t j = 0; j < group->entry_count; j++) {
                struct ini_entry *entry = &group->entries[j];
                if (strcmp(entry->key, key) == 0) {
                    c_group = i;
                    c_entry = j;
                    break;
                }
            }
        }
    }

    if (c_group == -1 || c_entry == -1) {
        return;
    }

    struct ini_group *group = &ini_file->groups[c_group];
    struct ini_entry *entries = remove_ini_entry_from_array(group->entries, group->entry_count, c_entry);
    group->entries = entries;
    group->entry_count--;
}

// decrement a count value as entry value by a given group and entry key
void decrement_value_for_group_and_key(struct ini_file *ini_file, const char *group_name, const char *key) {
    struct ini_group *group = find_group(ini_file, group_name);
    if (group != NULL) {
        for (int32_t j = 0; j < group->entry_count; j++) {
            struct ini_entry *entry = &group->entries[j];
            if (strcmp(entry->key, key) == 0) {
                int32_t new_count;
                sscanf(entry->value, "%d", &new_count);
                new_count--;
                const char *out_value = int32_to_str(new_count);
                (*entry).value = out_value;
                break;
            }
        }
    }
}

int32_t write_ini_file(struct ini_file *ini_file, const char *path, const char *filename) {

    const char *slidedat_filename = concat_path_filename(path, filename);
    file_handle *fp = file_open(slidedat_filename, "w");

    if (fp == NULL) {
        fprintf(stderr, "Error: Failed writing index file.\n");
        return -1;
    }

    for (int32_t i = 0; i < ini_file->group_count; i++) {
        struct ini_group group = ini_file->groups[i];
        const char *group_line = add_square_brackets(group.group_identifier);
        file_printf(fp, "%s\n", group_line);

        for (int32_t j = 0; j < group.entry_count; j++) {
            struct ini_entry entry = group.entries[j];
            const char *entry_line = add_equals_sign(entry.key, entry.value);
            file_printf(fp, "%s\n", entry_line);
        }
    }

    file_close(fp);
    return 0;
}