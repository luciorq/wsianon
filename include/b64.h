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
 * `b64.h' - b64
 *
 * copyright (c) 2014 joseph werle
 */

#ifndef HEADER_B64_H
#define HEADER_B64_H

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// #ifndef B64_H
#define B64_H 1

/**
 *  Memory allocation functions to use. You can define b64_malloc and
 * b64_realloc to custom functions if you want.
 */

#ifndef b64_malloc
#define b64_malloc(ptr) malloc(ptr)
#endif
#ifndef b64_realloc
#define b64_realloc(ptr, size) realloc(ptr, size)
#endif

// How much memory to allocate per buffer
#define B64_BUFFER_SIZE (1024 * 64) // 64K

// Start buffered memory
unsigned char *b64_buf_malloc();

// Update memory size. Returns the same pointer if we
// have enough space in the buffer. Otherwise, we add
// additional buffers.
unsigned char *b64_buf_realloc(unsigned char *ptr, size_t size);

/**
 * Decode `char *' source with `size_t' size.
 * Returns a `unsigned char *' base64 decoded string + size of decoded string.
 */
unsigned char *b64_decode_ex(const char *, size_t, size_t *);

/**
 * Encode `unsigned char *' source with `size_t' size.
 * Returns a `char *' base64 encoded string.
 */
unsigned char *b64_encode(const unsigned char *src, size_t len);

#endif