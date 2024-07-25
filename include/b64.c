/**
* The MIT License (MIT)
*
* Copyright (c) 2014 Little Star Media, Inc.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:

* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
*/

/**
 * copyright (c) 2014 joseph werle
 */

#include "b64.h"

/**
 * Base64 index table.
 */
static const char b64_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'};

// The number of buffers we need
size_t bufc = 0;

unsigned char *b64_buf_malloc() {
    unsigned char *buf = b64_malloc(B64_BUFFER_SIZE);
    bufc = 1;
    return buf;
}

unsigned char *b64_buf_realloc(unsigned char *ptr, size_t size) {
    if (size > bufc * B64_BUFFER_SIZE) {
        while (size > bufc * B64_BUFFER_SIZE)
            bufc++;
        unsigned char *buf = b64_realloc(ptr, B64_BUFFER_SIZE * bufc);
        if (!buf)
            return NULL;
        return buf;
    }

    return ptr;
}

// for decoding
void bytes_to_char(unsigned char *buffer, unsigned char *tmp) {
    buffer[0] = (tmp[0] << 2) + ((tmp[1] & 0x30) >> 4);
    buffer[1] = ((tmp[1] & 0xf) << 4) + ((tmp[2] & 0x3c) >> 2);
    buffer[2] = ((tmp[2] & 0x3) << 6) + tmp[3];
}

// for encoding
void six_bits_to_b64_char(unsigned char *buffer, unsigned char *tmp) {
    buffer[0] = (tmp[0] & 0xfc) >> 2;
    buffer[1] = ((tmp[0] & 0x03) << 4) + ((tmp[1] & 0xf0) >> 4);
    buffer[2] = ((tmp[1] & 0x0f) << 2) + ((tmp[2] & 0xc0) >> 6);
    buffer[3] = tmp[2] & 0x3f;
}

unsigned char *b64_decode_ex(const char *src, size_t len, size_t *decsize) {

    int32_t byte_length = 0;
    int32_t pos = 0;
    int32_t decoded_char = 0;
    size_t size = 0;
    int8_t num_of_chars_to_dec = 4;
    unsigned char *decoded_buffer = NULL;
    unsigned char buffer[3];
    unsigned char tmp[4];

    // alloc
    decoded_buffer = (unsigned char *)b64_buf_malloc();
    if (NULL == decoded_buffer) {
        return NULL;
    }

    // parse until end of source
    while (len--) {
        // break if char is '=' or not base64 char
        if (src[pos] == '=') {
            break;
        }
        if (!(isalnum(src[pos]) || src[pos] == '+' || src[pos] == '/')) {
            break;
        }

        // read up to 4 bytes at a time into 'tmp'
        tmp[byte_length++] = src[pos++];

        // if 4 bytes read then decode into 'buffer'
        if (byte_length == num_of_chars_to_dec) {
            // translate values in 'tmp' from table
            for (byte_length = 0; byte_length < num_of_chars_to_dec; byte_length++) {
                // find translation char in 'b64_table'
                for (decoded_char = 0; decoded_char < 64; decoded_char++) {
                    if (tmp[byte_length] == b64_table[decoded_char]) {
                        tmp[byte_length] = decoded_char;
                        break;
                    }
                }
            }

            // decode
            bytes_to_char(buffer, tmp);

            // write decoded buffer to 'decoded_buffer'
            decoded_buffer = (unsigned char *)b64_buf_realloc(decoded_buffer, size + 3);
            if (decoded_buffer != NULL) {
                for (byte_length = 0; byte_length < 3; byte_length++) {
                    decoded_buffer[size++] = buffer[byte_length];
                }
            } else {
                return NULL;
            }

            // reset
            byte_length = 0;
        }
    }

    // remainder
    if (byte_length > 0) {

        // fill 'tmp' with '\0' at most 4 times
        for (pos = byte_length; pos < 4; pos++) {
            tmp[pos] = '\0';
        }

        // translate remainder
        for (pos = 0; pos < 4; pos++) {
            // find translation char in 'b64_table'
            for (decoded_char = 0; decoded_char < 64; decoded_char++) {
                if (tmp[pos] == b64_table[decoded_char]) {
                    tmp[pos] = decoded_char;
                    break;
                }
            }
        }

        // decode
        bytes_to_char(buffer, tmp);

        // write remainer decoded buffer to 'decoded_buffer'
        decoded_buffer = (unsigned char *)b64_buf_realloc(decoded_buffer, size + (byte_length - 1));
        if (decoded_buffer != NULL) {
            for (pos = 0; (pos < byte_length - 1); pos++) {
                decoded_buffer[size++] = buffer[pos];
            }
        } else {
            return NULL;
        }
    }

    // Make sure we have enough space to add '\0' character at end.
    decoded_buffer = (unsigned char *)b64_buf_realloc(decoded_buffer, size + 1);
    if (decoded_buffer != NULL) {
        decoded_buffer[size] = '\0';
    } else {
        return NULL;
    }

    // set new size of decoded buffer
    if (decsize != NULL) {
        *decsize = size;
    }

    return decoded_buffer;
}

unsigned char *b64_encode(const unsigned char *src, size_t len) {
    int8_t byte_length = 0;
    unsigned char *encoded_buffer = NULL;
    int8_t num_of_chars_to_enc = 3;
    int8_t sections = 4;
    size_t size = 0;
    unsigned char buffer[sections];
    unsigned char tmp[num_of_chars_to_enc];

    // alloc
    encoded_buffer = (unsigned char *)b64_buf_malloc();
    if (NULL == encoded_buffer) {
        return NULL;
    }

    // parse until end of source
    while (len--) {
        // read up to 3 bytes at a time into 'tmp'
        tmp[byte_length++] = *(src++);

        // if 3 bytes read then encode into 'buffer'
        if (byte_length == num_of_chars_to_enc) {
            six_bits_to_b64_char(buffer, tmp);

            // allocate 4 new byts for 'encoded_buffer` and
            // then translate each encoded buffer
            // part by index from the base 64 index table
            // into 'encoded_buffer' unsigned char array
            encoded_buffer = (unsigned char *)b64_buf_realloc(encoded_buffer, size + sections);
            for (byte_length = 0; byte_length < sections; byte_length++) {
                encoded_buffer[size++] = b64_table[buffer[byte_length]];
            }

            // reset index
            byte_length = 0;
        }
    }

    // remainder
    if (byte_length > 0) {
        // fill 'tmp' with '\0' at most 3 times
        for (int32_t i = byte_length; i < num_of_chars_to_enc; i++) {
            tmp[i] = '\0';
        }

        // encode
        six_bits_to_b64_char(buffer, tmp);

        // perform same write to `encoded_buffer` with new allocation
        for (int32_t i = 0; (i < byte_length + 1); i++) {
            encoded_buffer = (unsigned char *)b64_buf_realloc(encoded_buffer, size + 1);
            encoded_buffer[size++] = b64_table[buffer[i]];
        }

        // while there is still a remainder
        // append '=' to 'encoded_buffer'
        while ((byte_length++ < num_of_chars_to_enc)) {
            encoded_buffer = (unsigned char *)b64_buf_realloc(encoded_buffer, size + 1);
            encoded_buffer[size++] = '=';
        }
    }

    // Make sure we have enough space to add '\0' character at end.
    encoded_buffer = (unsigned char *)b64_buf_realloc(encoded_buffer, size + 1);
    encoded_buffer[size] = '\0';

    return encoded_buffer;
}