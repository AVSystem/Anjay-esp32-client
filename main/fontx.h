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
#ifndef _FONTX_H_
#define _FONTX_H_

#include "sdkconfig.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#if CONFIG_ANJAY_CLIENT_LCD

#    define FontxGlyphBufSize (32 * 32 / 8)

typedef struct {
    const char *path;
    char fxname[10];
    bool opened;
    bool valid;
    bool is_ank;
    uint8_t w;
    uint8_t h;
    uint16_t fsz;
    uint8_t bc;
    FILE *file;
} FontxFile;

void AaddFontx(FontxFile *fx, const char *path);
void InitFontx(FontxFile *fxs, const char *f0, const char *f1);
bool OpenFontx(FontxFile *fx);
void CloseFontx(FontxFile *fx);
void DumpFontx(FontxFile *fxs);
uint8_t getFortWidth(FontxFile *fx);
uint8_t getFortHeight(FontxFile *fx);
bool GetFontx(FontxFile *fxs,
              uint8_t ascii,
              uint8_t *pGlyph,
              uint8_t *pw,
              uint8_t *ph);
void Font2Bitmap(
        uint8_t *fonts, uint8_t *line, uint8_t w, uint8_t h, uint8_t inverse);
void UnderlineBitmap(uint8_t *line, uint8_t w, uint8_t h);
void ReversBitmap(uint8_t *line, uint8_t w, uint8_t h);
void ShowFont(uint8_t *fonts, uint8_t pw, uint8_t ph);
void ShowBitmap(uint8_t *bitmap, uint8_t pw, uint8_t ph);
uint8_t RotateByte(uint8_t ch);

#endif /* CONFIG_ANJAY_CLIENT_LCD */
#endif /* _FONTX_H_ */
