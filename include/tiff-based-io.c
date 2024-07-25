#include "tiff-based-io.h"

// initialize directory array for a given tiff file
void init_tiff_file(struct tiff_file *file, size_t init_size) {
    size_t alloc_size = init_size * sizeof(struct tiff_directory);
    file->directories = (struct tiff_directory *)malloc(alloc_size);
    memset(file->directories, 0, alloc_size);
    file->used = 0;
    file->size = init_size;
}

// add directory to a given tiff file and resize array if necessary
void insert_dir_into_tiff_file(struct tiff_file *file, struct tiff_directory *dir) {
    // reallocate directories array dynamically
    if (file->used == file->size) {
        file->size *= 2;
        size_t realloc_size = file->size * sizeof(struct tiff_directory);
        file->directories = (struct tiff_directory *)realloc(file->directories, realloc_size);
        memset(&(file->directories[file->size / 2]), 0, realloc_size / 2);
    }
    file->directories[file->used++] = *dir;
    free(dir);
}

// free tiff_file with all directories and entries
void free_tiff_file(struct tiff_file *file) {
    free(file->directories);
    free(file);
}

// fix the byte order for data array depending on the endianess
// of the operating system and the tiff file
void fix_byte_order(void *data, int32_t size, int64_t count, bool big_endian) {
    // we only need to swap data, if system endianness
    // and tiff endianness are unequal
    if (is_system_big_endian() != big_endian) {
        switch (size) {
        case 1:
            break;
        case 2: {
            uint16_t *arr = (uint16_t *)data;
            for (int64_t i = 0; i < count; i++) {
                _swap_uint16(arr[i]);
            }
            break;
        }
        case 4: {
            uint32_t *arr = (uint32_t *)data;
            for (int64_t i = 0; i < count; i++) {
                _swap_uint32(arr[i]);
            }
            break;
        }
        case 8: {
            uint64_t *arr = (uint64_t *)data;
            for (int64_t i = 0; i < count; i++) {
                _swap_uint64(arr[i]);
            }
            break;
        }
        default:
            break;
        }
    }
}

// read an unsigned integer from a filestream at current position
uint64_t read_uint(file_handle *fp, int32_t size, bool big_endian) {
    uint8_t buffer[size];
    if (file_read(buffer, size, 1, fp) != 1) {
        return 0;
    }

    fix_byte_order(buffer, sizeof(buffer), 1, big_endian);

    switch (size) {
    case 1: {
        uint8_t result;
        memcpy(&result, buffer, sizeof(result));
        return result;
    }
    case 2: {
        uint16_t result;
        memcpy(&result, buffer, sizeof(result));
        return result;
    }
    case 4: {
        uint32_t result;
        memcpy(&result, buffer, sizeof(result));
        return result;
    }
    case 8: {
        uint64_t result;
        memcpy(&result, buffer, sizeof(result));
        return result;
    }
    default: {
        return 0;
    }
    }
}

// get the type size needed to readout the value
uint32_t get_size_of_value(uint16_t type, uint32_t *count) {
    if (type == TIFF_BYTE || type == TIFF_ASCII || type == TIFF_SBYTE || type == TIFF_UNDEFINED) {
        return 1;
    } else if (type == TIFF_SHORT || type == TIFF_SSHORT) {
        return 2;
    } else if (type == TIFF_LONG || type == TIFF_SLONG || type == TIFF_FLOAT || type == TIFF_IFD) {
        return 4;
    } else if (type == TIFF_RATIONAL || type == TIFF_SRATIONAL) {
        *count *= 2;
        return 4;
    } else if (type == TIFF_DOUBLE || type == TIFF_LONG8 || type == TIFF_SLONG8 || type == TIFF_IFD8) {
        return 8;
    } else {
        return 0;
    }
}

uint64_t fix_ndpi_offset(uint64_t directory_offset, uint64_t offset) {
    // we need to fix the ndpi offset to prevent the pointer from overflowing
    uint64_t new_offset = (directory_offset & ~(uint64_t)UINT32_MAX) | (offset & UINT32_MAX);
    if (new_offset >= directory_offset) {
        new_offset = min(new_offset - UINT32_MAX - 1, new_offset);
    }
    return new_offset;
}

void free_tiff_dir_and_entries(struct tiff_directory *tiff_dir, struct tiff_entry *entries, struct tiff_entry *entry) {
    free(tiff_dir);
    free(entries);
    free(entry);
}

// read a tiff directory at a certain offset
struct tiff_directory *read_tiff_directory(file_handle *fp, uint64_t *dir_offset, uint64_t *in_pointer_offset,
                                           bool big_tiff, bool ndpi, bool big_endian) {
    uint64_t offset = *dir_offset;
    *dir_offset = 0;

    // seek to directory offset
    if (file_seek(fp, offset, SEEK_SET) != 0) {
        fprintf(stderr, "Error: Seeking to offset failed.\n");
        return NULL;
    }

    // read number of entries in directory
    uint64_t entry_count = read_uint(fp, big_tiff ? 8 : 2, big_endian);

    struct tiff_directory *tiff_dir = (struct tiff_directory *)malloc(sizeof(struct tiff_directory));
    tiff_dir->count = entry_count;
    tiff_dir->in_pointer_offset = *in_pointer_offset;

    struct tiff_entry *entries = (struct tiff_entry *)malloc(entry_count * sizeof(struct tiff_entry));

    for (uint64_t i = 0; i < entry_count; i++) {
        struct tiff_entry *entry = (struct tiff_entry *)malloc(sizeof(struct tiff_entry));

        if (entry == NULL) {
            fprintf(stderr, "Error: Could not allocate memory for entry.\n");
            free_tiff_dir_and_entries(tiff_dir, entries, entry);
            return NULL;
        }

        entry->start = file_tell(fp);

        uint16_t tag = read_uint(fp, 2, big_endian);
        uint16_t type = read_uint(fp, 2, big_endian);
        uint64_t count = read_uint(fp, big_tiff ? 8 : 4, big_endian);
        entry->tag = tag;
        entry->type = type;
        entry->count = count;

        // calculate the size of the entry value
        uint32_t value_size = get_size_of_value(entry->type, &entry->count);
        if (!value_size || count > (SIZE_MAX / value_size)) {
            fprintf(stderr, "Error: Failed to determine valid parameters to read value from file.\n");
            free_tiff_dir_and_entries(tiff_dir, entries, entry);
            return NULL;
        }

        // read entry value to array
        uint8_t value[(big_tiff || ndpi) ? 8 : 4];
        uint8_t read_size = big_tiff ? 8 : 4;
        if (file_read(value, read_size, 1, fp) != 1) {
            fprintf(stderr, "Error: Reading value to array failed.\n");
            free_tiff_dir_and_entries(tiff_dir, entries, entry);
            return NULL;
        }

        if (ndpi) {
            bool is_value = (value_size * count <= read_size);
            if (file_seek(fp,
                          offset + (NDPI_ENTRY_EXTENSION * entry_count) + (NDPI_BIT_EXTENSION * i) +
                              (NDPI_ENTRY_EXTENSION / 2),
                          SEEK_SET) != 0) {
                fprintf(stderr, "Error: Cannot seek to offset extension.\n");
                free_tiff_dir_and_entries(tiff_dir, entries, entry);
                return NULL;
            }
            if (file_read(value + NDPI_BIT_EXTENSION, NDPI_BIT_EXTENSION, 1, fp) != 1) {
                fprintf(stderr, "Error: Cannot read offset extension.\n");
                free_tiff_dir_and_entries(tiff_dir, entries, entry);
                return NULL;
            }
            if (is_value && (value[4] > 0 || value[5] > 0 || value[6] > 0 || value[7] > 0)) {
                uint32_t result;
                memcpy(&result, value + NDPI_BIT_EXTENSION, sizeof(result));
                tiff_dir->ndpi_high_bits = result;
            }
            uint64_t dir_start = (NDPI_ENTRY_EXTENSION * (i + 1)) + (NDPI_BIT_EXTENSION / 2);
            if (file_seek(fp, offset + dir_start, SEEK_SET) != 0) {
                fprintf(stderr, "Error: Cannot seek to IFD start.\n");
                free_tiff_dir_and_entries(tiff_dir, entries, entry);
                return NULL;
            }
        }

        if (big_tiff || ndpi) {
            // big tiff or ndpi offset pointer reserves 8 bytes
            memcpy(&entry->offset, value, 8);
            fix_byte_order(&entry->offset, sizeof(entry->offset), 1, big_endian);
        } else {
            // non big tiff offset only reserves 4 bytes
            uint32_t offset32;
            memcpy(&offset32, value, 4);
            fix_byte_order(&offset32, sizeof(offset32), 1, big_endian);
            entry->offset = offset32;
        }

        entries[i] = *entry;
        free(entry);
    }

    // get the directory offset of the successor
    uint64_t next_dir_offset = file_tell(fp) + (big_tiff ? 8 : 2);

    tiff_dir->entries = entries;
    tiff_dir->out_pointer_offset = next_dir_offset;
    *dir_offset = read_uint(fp, (big_tiff || ndpi) ? 8 : 4, big_endian);

    return tiff_dir;
}

// check tiff file header
int32_t check_file_header(file_handle *fp, bool *big_endian, bool *big_tiff) {
    int32_t result = -1;
    uint16_t endianness;

    // read endianness in header
    if (file_read(&endianness, sizeof endianness, 1, fp) != 1) {
        return result;
    }

    if (endianness == TIFF_BIGENDIAN || endianness == TIFF_LITTLEENDIAN) {
        // set endianness and check ndpi version in header
        *big_endian = (endianness == TIFF_BIGENDIAN);
        uint16_t version = read_uint(fp, 2, *big_endian);

        if (version == TIFF_VERSION_BIG) {
            // try to read offset and padding for tiff version
            // if version is not classic
            uint16_t offset_size = read_uint(fp, 2, *big_endian);
            uint16_t pad = read_uint(fp, 2, *big_endian);
            if (offset_size == 8 && pad == 0) {
                *big_tiff = true;
                result = 0;
            }
        } else if (version == TIFF_VERSION_CLASSIC) {
            *big_tiff = false;
            result = 0;
        }
    }
    return result;
}

// read the tiff file structure with offsets from the file stream
struct tiff_file *read_tiff_file(file_handle *fp, bool big_tiff, bool ndpi, bool big_endian) {
    // get directory offset; file stream pointer must be located just
    // before the directory offset
    uint64_t in_pointer_offset = file_tell(fp);
    uint64_t diroff = read_uint(fp, (big_tiff || ndpi) ? 8 : 4, big_endian);
    // reading the initial directory
    struct tiff_directory *dir = read_tiff_directory(fp, &diroff, &in_pointer_offset, big_tiff, ndpi, big_endian);

    if (dir == NULL) {
        fprintf(stderr, "Error: Failed reading directory.\n");
        return NULL;
    }

    struct tiff_file *file = (struct tiff_file *)malloc(sizeof(struct tiff_file));
    // initialize tiff file and add first directory
    init_tiff_file(file, 1);
    insert_dir_into_tiff_file(file, dir);

    // when the directory offset is 0 we reached the end of the tiff file
    while (diroff != 0) {
        uint64_t current_in_pointer_offset = file_tell(fp) - 8;
        struct tiff_directory *current_dir =
            read_tiff_directory(fp, &diroff, &current_in_pointer_offset, big_tiff, ndpi, big_endian);

        if (current_dir == NULL) {
            fprintf(stderr, "Error: Failed reading directory.\n");
            free(current_dir);
            return NULL;
        }
        insert_dir_into_tiff_file(file, current_dir);
    }

    return file;
}

// check the head of the directory offset for a given prefix. if the head is not equal to the given
// prefix the label/macro image is not wiped
int32_t check_prefix(file_handle *fp, const char *prefix) {
    size_t prefix_len = strlen(prefix);
    char *buf = (char *)malloc(prefix_len + 1);
    buf[prefix_len] = '\0';

    if (file_read(buf, prefix_len, 1, fp) != 1) {
        fprintf(stderr, "Error: Could not read strip prefix.\n");
        free(buf);
        return -1;
    }

    if (strcmp(prefix, buf) != 0) {
        fprintf(stderr, "Error: Prefix in data strip not found.\n");
        free(buf);
        return -1;
    }
    free(buf);
    return 0;
}

int32_t wipe_directory(file_handle *fp, struct tiff_directory *dir, bool ndpi, bool big_endian, bool big_tiff,
                       const char *prefix, const char *suffix) {
    int32_t size_offsets;
    int32_t size_lengths;
    // gather strip offsets and lengths form tiff directory
    if (big_tiff) {
        uint64_t *strip_offsets = read_pointer64_by_tag(fp, dir, TIFFTAG_STRIPOFFSETS, ndpi, big_endian, &size_offsets);
        uint64_t *strip_lengths =
            read_pointer64_by_tag(fp, dir, TIFFTAG_STRIPBYTECOUNTS, ndpi, big_endian, &size_lengths);

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

            if (prefix != NULL) {
                if (check_prefix(fp, prefix) != 0) {
                    free(strip_offsets);
                    free(strip_lengths);
                    return -1;
                }
                file_seek(fp, strip_offsets[i], SEEK_SET);
            }

            // fill strip with zeros
            // ToDo: check if writing 0's is sufficient
            char *strip = create_pre_suffixed_char_array('0', strip_lengths[i], prefix, suffix);
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
    } else {
        uint32_t *strip_offsets = read_pointer32_by_tag(fp, dir, TIFFTAG_STRIPOFFSETS, ndpi, big_endian, &size_offsets);
        uint32_t *strip_lengths =
            read_pointer32_by_tag(fp, dir, TIFFTAG_STRIPBYTECOUNTS, ndpi, big_endian, &size_lengths);

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

            uint64_t new_offset = strip_offsets[i];

            // Fix NDPI offset in case file is larger than 4GB
            // convert to uint64, add high bits of ndpi dir to UINT32_MAX
            // add the strip offset to this in order to get the actual offset
            int64_t current_pos = file_tell(fp);
            file_seek(fp, 0, SEEK_END);
            int64_t size = file_tell(fp);
            file_seek(fp, current_pos, SEEK_SET);
            if (ndpi && (size > UINT32_MAX)) {
                new_offset = ((uint64_t)UINT32_MAX + dir->ndpi_high_bits) + (uint64_t)strip_offsets[i];
            }

            file_seek(fp, new_offset, SEEK_SET);

            if (prefix != NULL) {
                if (check_prefix(fp, prefix) != 0) {
                    free(strip_offsets);
                    free(strip_lengths);
                    return -1;
                }
                file_seek(fp, new_offset, SEEK_SET);
            }

            // fill strip with zeros
            // ToDo: check if writing 0's is sufficient
            char *strip = create_pre_suffixed_char_array('0', strip_lengths[i], prefix, suffix);
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
    }
    return 0;
}

// read a 32-bit pointer from the directory entries by tiff tag
uint32_t *read_pointer32_by_tag(file_handle *fp, struct tiff_directory *dir, int32_t tag, bool ndpi, bool big_endian,
                                int32_t *length) {
    for (uint64_t i = 0; i < dir->count; i++) {
        struct tiff_entry entry = dir->entries[i];
        if (entry.tag == tag) {
            int32_t entry_size = get_size_of_value(entry.type, &entry.count);

            if (entry_size) {
                uint32_t *v_buffer = (uint32_t *)malloc(entry_size * entry.count);

                if (entry.count == 1) {
                    *length = entry.count;
                    v_buffer[0] = entry.offset;
                    return v_buffer;
                }

                uint64_t new_offset = entry.offset;

                if (ndpi) {
                    new_offset = entry.start + 8;
                }

                if (file_seek(fp, new_offset, SEEK_SET)) {
                    fprintf(stderr, "Error: Failed to seek to offset %" PRIu64 ".\n", entry.offset);
                    continue;
                }
                if (file_read(v_buffer, entry_size, entry.count, fp) < 1) {
                    fprintf(stderr, "Error: Failed to read entry value.\n");
                    continue;
                }

                fix_byte_order(v_buffer, entry_size, entry.count, big_endian);
                *length = entry.count;

                return v_buffer;
            }
        }
    }
    return NULL;
}

// read a 64-bit pointer from the directory entries by tiff tag
uint64_t *read_pointer64_by_tag(file_handle *fp, struct tiff_directory *dir, int32_t tag, bool ndpi, bool big_endian,
                                int32_t *length) {
    for (uint64_t i = 0; i < dir->count; i++) {
        struct tiff_entry entry = dir->entries[i];
        if (entry.tag == tag) {
            int32_t entry_size = get_size_of_value(entry.type, &entry.count);

            if (entry_size) {
                uint64_t *v_buffer = (uint64_t *)malloc(entry_size * entry.count);

                if (entry.count == 1) {
                    *length = entry.count;
                    v_buffer[0] = entry.offset;
                    return v_buffer;
                }

                uint64_t new_offset = entry.offset;

                if (ndpi) {
                    new_offset = entry.start + 8;
                }

                if (file_seek(fp, new_offset, SEEK_SET)) {
                    fprintf(stderr, "Error: Failed to seek to offset %" PRIu64 ".\n", entry.offset);
                    continue;
                }
                if (file_read(v_buffer, entry_size, entry.count, fp) < 1) {
                    fprintf(stderr, "Error: Failed to read entry value.\n");
                    continue;
                }

                fix_byte_order(v_buffer, entry_size, entry.count, big_endian);
                *length = entry.count;

                return v_buffer;
            }
        }
    }

    return NULL;
}

int32_t unlink_directory(file_handle *fp, struct tiff_file *file, int32_t current_dir, bool is_ndpi) {

    if (current_dir == -1) {
        return -1;
    }

    struct tiff_directory dir = file->directories[current_dir];
    struct tiff_directory successor = file->directories[current_dir + 1];

    if (!is_ndpi && successor.count == 0 && successor.in_pointer_offset == 0) {
        // current directory is the last in file
        // search to out pointer of current dir
        if (file_seek(fp, dir.out_pointer_offset, SEEK_SET)) {
            fprintf(stderr, "Error: Failed to seek to offset.\n");
            return -1;
        }
        // overwrite out pointer with 0 to end file
        uint64_t new_pointer_address[1];
        new_pointer_address[0] = 0x0;
        if (file_write(new_pointer_address, sizeof(uint64_t), 1, fp) != 1) {
            fprintf(stderr, "Error: Failed to write directory out pointer \
                        to null at pointer position.\n");
            return -1;
        }
        return 0;
    }

    // current directory has a successor
    if (file_seek(fp, successor.in_pointer_offset, SEEK_SET)) {
        fprintf(stderr, "Error: Failed to seek to offset.\n");
        return -1;
    }
    uint64_t new_pointer_address[1];
    if (file_read(&new_pointer_address, sizeof(uint64_t), 1, fp) != 1) {
        fprintf(stderr, "Error: Failed to read pointer.\n");
        return -1;
    }
    if (file_seek(fp, dir.in_pointer_offset, SEEK_SET)) {
        fprintf(stderr, "Error: Failed to seek to offset.\n");
        return -1;
    }
    if (file_write(new_pointer_address, sizeof(uint64_t), 1, fp) != 1) {
        fprintf(stderr, "Error: Failed to write directory in pointer \
                    to predecessor at pointer position.\n");
        return -1;
    }

    return 0;
}

int32_t get_aperio_gt450_dir_by_name(struct tiff_file *file, const char *dir_name) {
    for (uint64_t i = 0; i < file->used; i++) {
        struct tiff_directory dir = file->directories[i];
        // printf("--directory %i\n", i);
        for (uint64_t j = 0; j < dir.count; j++) {
            struct tiff_entry entry = dir.entries[j];
            if (entry.tag == TIFFTAG_SUBFILETYPE) {
                if (entry.offset == 0) { // thumbnail or else
                    break;               // skip IFD
                }

                if ((strcmp(dir_name, LABEL) == 0 && entry.offset == 1) ||
                    (strcmp(dir_name, MACRO) == 0 && entry.offset == 9)) {
                    return i;
                }
            }
        }
    }
    return -1;
}

int32_t tag_value_contains(file_handle *fp, struct tiff_file *file, int32_t tag, const char *contains_value) {
    for (uint64_t i = 0; i < file->used; i++) {
        struct tiff_directory dir = file->directories[i];
        for (uint64_t j = 0; j < dir.count; j++) {
            struct tiff_entry entry = dir.entries[j];
            if (entry.tag == tag) {
                // get requested image tag from file
                file_seek(fp, entry.offset, SEEK_SET);
                int32_t entry_size = get_size_of_value(entry.type, &entry.count);

                char *buffer = malloc(entry_size * entry.count);
                if (file_read(buffer, entry.count, entry_size, fp) != 1) {
                    fprintf(stderr, "Error: Could not read image tag %" PRId32 ".\n", tag);
                    free(buffer);
                    return -1;
                }
                // check if tag value contains given string
                if (contains(buffer, contains_value)) {
                    free(buffer);
                    return 1;
                }
                free(buffer);
            }
        }
    }
    return -1;
}

int32_t get_directory_by_tag_and_value(file_handle *fp, struct tiff_file *file, int32_t tag, const char *value) {
    for (uint64_t i = 0; i < file->used; i++) {
        struct tiff_directory dir = file->directories[i];
        for (uint64_t j = 0; j < dir.count; j++) {
            struct tiff_entry entry = dir.entries[j];
            if (entry.tag == tag) {
                // get the tag value from file
                file_seek(fp, entry.offset, SEEK_SET);
                int32_t entry_size = get_size_of_value(entry.type, &entry.count);

                char *buffer = malloc(entry_size * entry.count);
                if (file_read(buffer, entry.count, entry_size, fp) != 1) {
                    fprintf(stderr, "Error: Could not read image tag %" PRId32 ".\n", tag);
                    free(buffer);
                    return -1;
                }

                // check if value contains expected value and return directory
                if (contains(buffer, value)) {
                    free(buffer);
                    return i;
                }
                free(buffer);
            }
        }
    }
    return -1;
}
