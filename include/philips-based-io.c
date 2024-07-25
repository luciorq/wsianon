#include "philips-based-io.h"

// replaces section of passed attribute with empty string
char *wipe_section_of_attribute(char *buffer, char *attribute) {
    const char *concatenated_str = concat_str(PHILIPS_ATT_END, PHILIPS_CLOSING_SYMBOL);
    char *rough_section = get_string_between_delimiters(buffer, attribute, concatenated_str);

    const char *concatenated_section = concat_str(attribute, rough_section);
    const char *concatenated_symbols = concat_str(PHILIPS_ATT_END, PHILIPS_CLOSING_SYMBOL);
    const char *section = concat_str(concatenated_section, concatenated_symbols);
    char *replacement = create_replacement_string(' ', strlen(section));
    char *result = replace_str(buffer, section, replacement);

    free((void *)concatenated_str);
    free(rough_section);
    free((void *)concatenated_section);
    free((void *)section);
    free((void *)concatenated_symbols);
    free(replacement);

    return result;
}

// returns value for an attribute
char *get_value_from_attribute(char *buffer, char *attribute) {
    char *value = get_string_between_delimiters(buffer, attribute, PHILIPS_ATT_OPEN);
    char *delimiter = get_string_between_delimiters(value, PHILIPS_ATT_PMSVR, PHILIPS_CLOSING_SYMBOL);

    // check for datatype
    if (strcmp(delimiter, PHILIPS_DELIMITER_STR) == 0) {
        const char *concatened_str = concat_str(PHILIPS_DELIMITER_STR, PHILIPS_CLOSING_SYMBOL);
        char *result = get_string_between_delimiters(value, concatened_str, PHILIPS_ATT_END);
        free(value);
        free(delimiter);
        free((void *)concatened_str);
        return result;
    } else if (strcmp(delimiter, PHILIPS_DELIMITER_INT) == 0) {
        const char *concatened_str = concat_str(PHILIPS_DELIMITER_INT, PHILIPS_CLOSING_SYMBOL);
        char *result = get_string_between_delimiters(value, concatened_str, PHILIPS_ATT_END);
        free(value);
        free(delimiter);
        free((void *)concatened_str);
        return result;
    } else {
        fprintf(stderr, "Unable find value for attribute with this datatype");
        free(value);
        free(delimiter);
        return NULL;
    }
}

// searches for attribute and replaces its value with equal amount of X's
char *anonymize_value_of_attribute(char *buffer, char *attribute) {
    char *rough_value = get_string_between_delimiters(buffer, attribute, PHILIPS_ATT_OPEN);
    const char *concatenated_str = concat_str(PHILIPS_DELIMITER_STR, PHILIPS_CLOSING_SYMBOL);
    char *value = get_string_between_delimiters(rough_value, concatenated_str, PHILIPS_ATT_END);
    // check for empty String
    if (strcmp(value, "") != 0) {
        char replace_with = 'X';
        char *replacement = create_replacement_string(replace_with, strlen(value));
        char *result = replace_str(buffer, value, replacement);
        strcpy(buffer, result);
        free(replacement);
        free(result);
    }
    free(rough_value);
    free((void *)concatenated_str);
    free(value);

    return buffer;
}

int32_t *get_height_and_width(const char *image_data) {

    // decode base64 string for image data
    size_t decode_size[1];
    decode_size[0] = strlen(image_data);
    unsigned char *decoded_data = b64_decode_ex(image_data, *decode_size, &decode_size[0]);

    // offset and length of bytes for width and height
    size_t offset = 0;
    size_t size_bytes_len = 0;

    // check structure for width and height
    for (size_t i = 0; i < decode_size[0]; i++) {

        // check prefix of possible SOF section
        if (decoded_data[i] == 255         // xff
            && decoded_data[i + 1] == 192  // xc0
            && decoded_data[i + 2] == 0    // x00
            && decoded_data[i + 3] == 17   // x11
            && decoded_data[i + 4] == 8) { // x08

            // prefix length
            offset = i + 5;

            // check suffix
            for (size_t j = offset; j < decode_size[0]; j++) {
                if (decoded_data[j] == 3 &&     // x03
                    decoded_data[j + 1] == 1) { // x01

                    bool is_suffix = false;

                    for (size_t z = j + 2; z < decode_size[0]; z++) {

                        // if another /x03/x01 was found, it is not SOF section
                        if (decoded_data[z] == 3 &&     // x03
                            decoded_data[z + 1] == 1) { // x01
                            break;
                        }

                        // if next section (huffman table) was found, then it is SOF section
                        if (decoded_data[z] == 255 &&     // xff
                            decoded_data[z + 1] == 196) { // xc4
                            is_suffix = true;
                            break;
                        }
                    }

                    // if suffix was found
                    if (is_suffix) {
                        break;
                    }
                }
                size_bytes_len++;
            }
            break;
        }
    }

    // alloc width and height
    int32_t div = size_bytes_len / 2;
    unsigned char *width_arr = (unsigned char *)malloc(sizeof(unsigned char) * div);
    unsigned char *height_arr = (unsigned char *)malloc(sizeof(unsigned char) * div);

    int32_t height;
    int32_t width;

    // if offset could not be found for either images or length of bytes are not equal, set width
    // and height to 1 in order to still anonymize image
    if (offset == 0 || size_bytes_len % 2 != 0) {
        height = 1;
        width = 1;
    } else {

        // set values
        for (int32_t i = 0; i < div; i++) {
            width_arr[i] = decoded_data[offset + i];
            height_arr[i] = decoded_data[offset + div + i];
        }

        // convert bytes into int

        height = bytes_to_int(height_arr, div);
        width = bytes_to_int(width_arr, div);
    }

    // declare and initialize array
    int32_t *h_and_w = malloc(sizeof(int32_t) * 2);
    h_and_w[0] = height;
    h_and_w[1] = width;

    free(height_arr);
    free(width_arr);
    free(decoded_data);

    return h_and_w;
}