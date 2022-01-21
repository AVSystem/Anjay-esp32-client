/*
 * Copyright 2021-2022 AVSystem <avsystem@avsystem.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * MIT License
 *
 * Copyright (c) 2020 nopnop2002
 *  - https://github.com/nopnop2002/esp-idf-m5stickC-Plus
 * Copyright (c) 2021-2022 AVSystem
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef _BMPFILE_H_
#define _BMPFILE_H_

#include "sdkconfig.h"
#include <stdint.h>

#if CONFIG_ANJAY_CLIENT_LCD

#    pragma pack(push, 1)
typedef struct {
    uint8_t magic[2];  /* the magic number used to identify the BMP file:
                          0x42 0x4D (Hex code points for B and M).
                          The following entries are possible:
                          BM - Windows 3.1x, 95, NT, ... etc
                          BA - OS/2 Bitmap Array
                          CI - OS/2 Color Icon
                          CP - OS/2 Color Pointer
                          IC - OS/2 Icon
                          PT - OS/2 Pointer. */
    uint32_t filesz;   /* the size of the BMP file in bytes */
    uint16_t creator1; /* reserved. */
    uint16_t creator2; /* reserved. */
    uint32_t offset;   /* the offset, i.e. starting address,
                          of the byte where the bitmap data can be found. */
} bmp_header_t;

typedef struct {
    uint32_t header_sz;     /* the size of this header (40 bytes) */
    uint32_t width;         /* the bitmap width in pixels */
    uint32_t height;        /* the bitmap height in pixels */
    uint16_t nplanes;       /* the number of color planes being used.
                   Must be set to 1. */
    uint16_t depth;         /* the number of bits per pixel,
                   which is the color depth of the image.
                   Typical values are 1, 4, 8, 16, 24 and 32. */
    uint32_t compress_type; /* the compression method being used.
                   See also bmp_compression_method_t. */
    uint32_t bmp_bytesz; /* the image size. This is the size of the raw bitmap
                data (see below), and should not be confused
                with the file size. */
    uint32_t hres;       /* the horizontal resolution of the image.
                (pixel per meter) */
    uint32_t vres;       /* the vertical resolution of the image.
                (pixel per meter) */
    uint32_t ncolors;    /* the number of colors in the color palette,
                or 0 to default to 2<sup><i>n</i></sup>. */
    uint32_t nimpcolors; /* the number of important colors used,
                or 0 when every color is important;
                generally ignored. */
} bmp_dib_v3_header_t;

typedef struct {
    bmp_header_t header;
    bmp_dib_v3_header_t dib;
} bmpfile_t;
#    pragma pack(pop)

#endif /* CONFIG_ANJAY_CLIENT_LCD */
#endif /* _BMPFILE_H_ */
