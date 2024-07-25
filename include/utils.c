#include "utils.h"
#include <inttypes.h>

// split a string by a given delimiter
char **str_split(char *a_str, const char a_delim) {
    char **result = 0;
    size_t count = 0;
    const char *tmp = a_str;
    const char *last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    // count the number of elements we need to extract
    while (*tmp) {
        if (a_delim == *tmp) {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    // add space for trailing token
    count += last_comma < (a_str + strlen(a_str) - 1);

    // add space for terminating null string so caller
    // knows where the list of returned strings ends
    count++;

    result = (char **)malloc(sizeof(char *) * count);

    if (result) {
        // fill result array
        size_t idx = 0;
        char *token = strtok(a_str, delim);

        while (token) {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == count - 1);
        *(result + idx) = 0;
    }

    return result;
}

// get the filename extension for a given filename
const char *get_filename_ext(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if (!dot || dot == filename)
        return "";
    return dot + 1;
}

int32_t file_exists(const char *filename) {
    file_handle *file;
    if ((file = file_open(filename, "rb+"))) {
        file_close(file);
        return 1;
    }
    return 0;
}

// return a char buffer filled with char x
char *create_pre_suffixed_char_array(const char fill_character, uint64_t length, const char *prefix,
                                     const char *suffix) {
    // assure length is a postive integer and not zero
    if (length < 1) {
        fprintf(stderr, "Error: Length smaller 1.\n");
        return NULL;
    }

    int32_t prefix_length = 0;
    int32_t suffix_length = 0;

    if (prefix != NULL) {
        prefix_length = strlen(prefix);
    }

    if (suffix != NULL) {
        suffix_length = strlen(suffix);
    }

    // allocate buffer
    char *result =
        (char *)malloc((length * sizeof(char)) + prefix_length * sizeof(char) + suffix_length * sizeof(char));

    int32_t start = 0;
    // add image prefix
    for (; start <= prefix_length - 1; start++) {
        result[start] = prefix[start];
    }

    // fill body of image with x
    for (; (uint64_t)start < length - suffix_length; start++) {
        result[start] = fill_character;
    }

    // add suffix to image
    for (int32_t i = 0; i < suffix_length; i++) {
        result[start + i] = suffix[i];
    }

    return result;
}

char *create_random_string(uint64_t length) {
    if (length < 1) {
        fprintf(stderr, "Error: Length smaller 1.\n");
        return NULL;
    }

    char *result = (char *)malloc(length + 1);

    srand(time(NULL));
    for (int32_t start = 0; (uint64_t)start < length; start++) {
        result[start] = *(int32_to_str(rand() % (9 - 1)));
    }

    result[length] = '\0';

    return result;
}

char *create_replacement_string(const char x, uint64_t length) {
    if (length < 1) {
        fprintf(stderr, "Warning: Length smaller 1. Empty value cannot be replaced.\n");
        return NULL;
    }

    char *result = (char *)malloc(length + 1);

    for (int32_t start = 0; (uint64_t)start < length; start++) {
        result[start] = x;
    }

    result[length] = '\0';

    return result;
}

char *replace_str(const char *original_str, const char *replace_str, const char *with_str) {
    char *result;
    char *tmp;
    int32_t length_front;
    int32_t count;

    if (!original_str || !replace_str || !with_str) {
        return NULL;
    }
    int32_t length_rep = strlen(replace_str);
    int32_t length_with = strlen(with_str);

    if (length_rep == 0 || length_with == 0) {
        return NULL;
    }

    const char *ins = original_str;
    for (count = 0; (tmp = strstr(ins, replace_str)); ++count) {
        ins = tmp + length_rep;
    }

    tmp = result = malloc(strlen(original_str) + (length_with - length_rep) * count + 1);

    if (!result) {
        return NULL;
    }

    while (count--) {
        ins = strstr(original_str, replace_str);
        length_front = ins - original_str;
        tmp = strncpy(tmp, original_str, length_front) + length_front;
        tmp = strcpy(tmp, with_str) + length_with;
        original_str += length_front + length_rep;
    }

    strcpy(tmp, original_str);
    return result;
}

void replace_str_inplace(char *original_str, const char *replace_str, const char *with_str) {
    int length_original_str = strlen(original_str);
    int length_replace_str = strlen(replace_str);
    int length_with_str = strlen(with_str);

    if (length_replace_str != length_with_str) {
        // lengths do not match so we simply return
        return;
    }

    char *pos = strstr(original_str, replace_str);

    if (pos == NULL) {
        // string was not found
        return;
    }

    memmove(pos + length_with_str, pos + length_replace_str,
            length_original_str - (pos - original_str) - length_replace_str + 1);
    memcpy(pos, with_str, length_with_str);
}

bool starts_with(const char *str, const char *pre) {
    return strlen(str) < strlen(pre) ? false : memcmp(pre, str, strlen(pre)) == 0;
}

char *get_string_between_delimiters(const char *buffer, const char *delimiter1, const char *delimiter2) {
    const char *substring1 = strstr(buffer, delimiter1);
    if (substring1) {
        const size_t s_del1 = strlen(delimiter1);
        if (delimiter2) {
            const char *substring2 = strstr(substring1 + s_del1, delimiter2);
            const size_t mlen = substring2 - (substring1 + s_del1);
            char *result = (char *)malloc(sizeof(char) * (mlen + 1)); // Buffer overflow here
            if (result) {
                memcpy(result, substring1 + s_del1, mlen);
                result[mlen] = '\0';
                return result;
            }
            free(result);
        }
    }
    return NULL;
}

void remove_leading_spaces(char *str) {
    char *p = str;
    int32_t l = strlen(p);

    while (isspace(p[l - 1]))
        p[--l] = 0;
    while (*p && isspace(*p))
        ++p, --l;

    memmove(str, p, l + 1);
}

const char *concat_str(const char *str1, const char *str2) {
    char *new_string = malloc(strlen(str1) + strlen(str2) + 2);
    strcpy(new_string, str1);
    strcat(new_string, str2);
    return new_string;
}

const char *concat_path_filename(const char *path, const char *filename) {
    char *new_string = malloc(strlen(path) + strlen(filename) + 3);
    strcpy(new_string, path);
    strcat(new_string, "/");
    strcat(new_string, filename);
    return new_string;
}

const char *concat_path_filename_ext(const char *path, const char *filename, const char *ext) {
    char *new_string = (char *)malloc(strlen(path) + strlen(filename) + strlen(ext) + 2);
    strcpy(new_string, path);
    // we assume that path is already ending with a dash
    strcat(new_string, filename);
    // strcat(new_string, ".");
    strcat(new_string, ext);
    return new_string;
}

const char *get_filename_from_path(const char *path) {
    if (path == NULL)
        return NULL;

    const char *temp_path = path;
    for (const char *p_cur = path; *p_cur != '\0'; p_cur++) {
        if (*p_cur == '/' || *p_cur == '\\')
            temp_path = p_cur + 1;
    }

    return temp_path;
}

const char *int32_to_str(int32_t integer) {
    int32_t length = snprintf(NULL, 0, "%d", integer);
    char *str = (char *)malloc(length + 1);
    snprintf(str, length + 1, "%d", integer);
    return str;
}

int32_t number_of_digits(int32_t integer) {
    int32_t result = (integer < 0) ? 2 : 1;
    while ((integer /= 10) != 0) {
        result++;
    }
    return result;
}

// put a given string in square brackets
const char *add_square_brackets(const char *str) {
    int32_t length = strlen(str) + sizeof("[") + sizeof("]") + sizeof("\0") + 1;
    char *result = (char *)malloc(length);
    strcpy(result, "[");
    strcat(result, str);
    strcat(result, "]");
    strcat(result, "\0");
    return result;
}

// add an equal sign with whitespaces between to strings
const char *add_equals_sign(const char *str1, const char *str2) {
    int32_t length = strlen(str1) + strlen(str2) + sizeof(" = ") + sizeof("\0") + 1;
    char *result = (char *)malloc(length);
    strcpy(result, str1);
    strcat(result, " = ");
    strcat(result, str2);
    strcat(result, "\0");
    return result;
}

// function to check if str1 contains str2
bool contains(const char *str1, const char *str2) { return strstr(str1, str2) != NULL; }

// function to check how many times str1 contains str2
int32_t count_contains(const char *str1, const char *str2) {
    int32_t i = 0, j = 0, count = 0;

    while (str1[i] != '\0') {
        if (str1[i] == str2[j]) {
            while (str1[i] == str2[j] && str2[j] != '\0') {
                j++;
                i++;
            }
            if (str2[j] == '\0') {
                count++;
            }
            j = 0;
        }
        i++;
    }
    return count;
}

const char *duplicate_file(const char *filename, const char *new_file_name, const char *file_extension) {
    // retrive filename from whole file path
    const char *_filename = get_filename_from_path(filename);

    if (_filename == NULL) {
        fprintf(stderr, "Error: Could not retrieve filename from filepath.\n");
        return NULL;
    }

    // get the directory path
    int32_t l_filename = strlen(filename);
    int32_t diff = l_filename - strlen(_filename);
    char path[diff + 1];
    memcpy(path, &filename[0], diff);
    path[diff] = '\0';

    const char *new_filename;
    // now we can concat the new filename
    if (new_file_name == NULL) {
        // if no label is given, we give the file a generic name
        const char *dummy_filename = "anonymized-wsi";
        new_filename = concat_path_filename_ext(path, dummy_filename, file_extension);
    } else {
        new_filename = concat_path_filename_ext(path, new_file_name, file_extension);
    }

    // we copy the file in our current directory
    // create a subfolder /output/?
    if (new_filename != NULL && copy_file_v2(filename, new_filename) == 0) {
        return new_filename;
    } else {
        return NULL;
    }
}

int32_t copy_file_v2(const char *src, const char *dest) {
    char command[strlen(src) + strlen(dest) + 15];
#ifdef __linux__
    // we create the copy command for linux
    snprintf(command, sizeof command, "cp \"%s\" \"%s\"%c", src, dest, '\0');
    return system(command);
#elif defined(__APPLE__) && defined(__MACH__)
    // we create the copy command for MacOS
    snprintf(command, sizeof command, "cp \"%s\" \"%s\"%c", src, dest, '\0');
    return system(command);
#elif _WIN32
    // we create the copy command for win32
    snprintf(command, sizeof command, "xcopy /Y \"%s\" \"%s\"%c", src, dest, '\0');
    return system(command);
#elif _WIN64
    // we create the copy command for win64
    snprintf(command, sizeof command, "xcopy /Y \"%s\" \"%s\"%c", src, dest, '\0');
    return system(command);
#else
    // todo: implement for mac
    return -1;
#endif
}

int32_t copy_directory(const char *src, const char *dest) {
    char command[strlen(src) + strlen(dest) + 15];
#ifdef __linux__
    // we create the copy command for linux
    snprintf(command, sizeof command, "cp -r \"%s\" \"%s\"%c", src, dest, '\0');
    return system(command);
#elif defined(__APPLE__) && defined(__MACH__)
    // we create the copy command for MacOS
    snprintf(command, sizeof command, "cp -r \"%s\" \"%s\"%c", src, dest, '\0');
    return system(command);
#elif _WIN32
    // we create the copy command for win32
    snprintf(command, sizeof command, "xcopy /Y \"%s\" \"%s\" /s /e%c", src, dest, '\0');
    return system(command);
#elif _WIN64
    // we create the copy command for win64
    snprintf(command, sizeof command, "xcopy /Y \"%s\" \"%s\" /s /e%c", src, dest, '\0');
    return system(command);
#else
    // todo: implement for mac
    return -1;
#endif
}

// determine wether the operating system
// is big or little endian
bool is_system_big_endian() {
    int32_t n = 1;
    if (*(char *)&n == 1) {
        return true;
    }
    return false;
}

// swap bytes of unsigned integer 16 bits
uint16_t _swap_uint16(uint16_t value) { return (value << 8) | (value >> 8); }

// swap bytes of unsigned integer 32 bits
uint32_t _swap_uint32(uint32_t value) {
    value = ((value << 8) & 0xFF00FF00) | ((value >> 8) & 0xFF00FF);
    return (value << 16) | (value >> 16);
}

// swap bytes of unsigned integer 64 bits
uint64_t _swap_uint64(uint64_t value) {
    value = ((value << 8) & 0xFF00FF00FF00FF00ULL) | ((value >> 8) & 0x00FF00FF00FF00FFULL);
    value = ((value << 16) & 0xFFFF0000FFFF0000ULL) | ((value >> 16) & 0x0000FFFF0000FFFFULL);
    return (value << 32) | (value >> 32);
}

const char *slice_str(const char *str, size_t start, size_t end) {
    char *result = (char *)malloc(end - start);
    size_t j = 0;
    for (size_t i = start; i <= end; ++i) {
        result[j++] = str[i];
    }
    result[j] = '\0';
    return result;
}

// convert bytes into int
int32_t bytes_to_int(unsigned char *buffer, int32_t size) {
    int32_t ret = 0;
    int32_t shift = 0;
    for (int32_t i = size - 1; i >= 0; i--) {
        ret |= (buffer[i]) << shift;
        shift += 8;
    }
    return ret;
}

// iteratively searches file for value and returns size of file where value was found
uint64_t iteratively_get_size_of_value(file_handle *fp, char *value, uint64_t step) {

    file_seek(fp, 0, SEEK_END);
    uint64_t file_length = file_tell(fp);

    while (step < file_length) {
        char *buffer = malloc(sizeof(char) * step);
        file_seek(fp, 0, SEEK_SET);

        if (file_read(buffer, step, 1, fp) != 1) {
            free(buffer);
            fprintf(stderr, "Error: Could not read file.\n");
            return 0;
        }

        // found value
        if (contains(buffer, value)) {
            // calculates size
            char *ret = strstr(buffer, value);
            uint64_t size = ret - buffer;
            file_seek(fp, 0, SEEK_SET);
            free(buffer);
            return size;
        } else {
            free(buffer);
            step += step;
        }
    }
    return 0;
}

const char *concat_wildcard_string_int32(const char *str, int32_t integer) {
    char *result_string = (char *)malloc(strlen(str) + number_of_digits(integer) + 1);
    sprintf(result_string, str, integer);
    return result_string;
}

const char *concat_wildcard_string_m_int32(const char *str, int32_t integer1, int32_t integer2) {
    char *result_string = (char *)malloc(strlen(str) + number_of_digits(integer1) + number_of_digits(integer2) + 1);
    sprintf(result_string, str, integer1, integer2);
    return result_string;
}

// read a signed integer 32 from file stream
int32_t *read_int32(file_handle *fp) {
    int32_t *buffer = (int32_t *)malloc(sizeof(int32_t));

    if (file_read(buffer, sizeof(*buffer), 1, fp) != 1) {
        free(buffer);
        return NULL;
    }

    return buffer;
}

// assert a int32_t has a certain value
// used to skip pointer position on stream
bool assert_value(file_handle *fp, int32_t value) {
    int32_t *v_to_check = read_int32(fp);
    return *v_to_check == value;
}
