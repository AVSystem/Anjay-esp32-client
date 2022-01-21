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

#include "fontx.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "sdkconfig.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/unistd.h>

#if CONFIG_ANJAY_CLIENT_LCD

void AddFontx(FontxFile *fx, const char *path) {
    memset(fx, 0, sizeof(FontxFile));
    fx->path = path;
    fx->opened = false;
}

void InitFontx(FontxFile *fxs, const char *f0, const char *f1) {
    AddFontx(&fxs[0], f0);
    AddFontx(&fxs[1], f1);
}

bool OpenFontx(FontxFile *fx) {
    FILE *f;
    if (!fx->opened) {
        f = fopen(fx->path, "r");
        if (f == NULL) {
            fx->valid = false;
            printf("Fontx:%s not found.\n", fx->path);
            return fx->valid;
        }
        fx->opened = true;
        fx->file = f;
        char buf[18];
        if (fread(buf, 1, sizeof(buf), fx->file) != sizeof(buf)) {
            fx->valid = false;
            printf("Fontx:%s not FONTX format.\n", fx->path);
            fclose(fx->file);
            return fx->valid;
        }

        memcpy(fx->fxname, &buf[6], 8);
        fx->w = buf[14];
        fx->h = buf[15];
        fx->is_ank = (buf[16] == 0);
        fx->bc = buf[17];
        fx->fsz = (fx->w + 7) / 8 * fx->h;
        if (fx->fsz > FontxGlyphBufSize) {
            printf("Fontx:%s is too big font size.\n", fx->path);
            fx->valid = false;
            fclose(fx->file);
            return fx->valid;
        }
        fx->valid = true;
    }
    return fx->valid;
}

void CloseFontx(FontxFile *fx) {
    if (fx->opened) {
        fclose(fx->file);
        fx->opened = false;
    }
}

void DumpFontx(FontxFile *fxs) {
    for (int i = 0; i < 2; i++) {
        printf("fxs[%d]->path=%s\n", i, fxs[i].path);
        printf("fxs[%d]->opened=%d\n", i, fxs[i].opened);
        printf("fxs[%d]->fxname=%s\n", i, fxs[i].fxname);
        printf("fxs[%d]->valid=%d\n", i, fxs[i].valid);
        printf("fxs[%d]->is_ank=%d\n", i, fxs[i].is_ank);
        printf("fxs[%d]->w=%d\n", i, fxs[i].w);
        printf("fxs[%d]->h=%d\n", i, fxs[i].h);
        printf("fxs[%d]->fsz=%d\n", i, fxs[i].fsz);
        printf("fxs[%d]->bc=%d\n", i, fxs[i].bc);
    }
}

uint8_t getFortWidth(FontxFile *fx) {
    printf("fx->w=%d\n", fx->w);
    return (fx->w);
}

uint8_t getFortHeight(FontxFile *fx) {
    printf("fx->h=%d\n", fx->h);
    return (fx->h);
}

bool GetFontx(FontxFile *fxs,
              uint8_t ascii,
              uint8_t *pGlyph,
              uint8_t *pw,
              uint8_t *ph) {

    int i;
    uint32_t offset;

    for (i = 0; i < 2; i++) {
        if (!OpenFontx(&fxs[i]))
            continue;

        if (ascii < 0x80) {
            if (fxs[i].is_ank) {
                offset = 17 + ascii * fxs[i].fsz;
                if (fseek(fxs[i].file, offset, SEEK_SET)) {
                    printf("Fontx:seek(%u) failed.\n", offset);
                    return false;
                }
                if (fread(pGlyph, 1, fxs[i].fsz, fxs[i].file) != fxs[i].fsz) {
                    printf("Fontx:fread failed.\n");
                    return false;
                }
                if (pw)
                    *pw = fxs[i].w;
                if (ph)
                    *ph = fxs[i].h;
                return true;
            }
        }
    }
    return false;
}

void Font2Bitmap(
        uint8_t *fonts, uint8_t *line, uint8_t w, uint8_t h, uint8_t inverse) {
    int x, y;
    for (y = 0; y < (h / 8); y++) {
        for (x = 0; x < w; x++) {
            line[y * 32 + x] = 0;
        }
    }

    int mask = 7;
    int fontp;
    fontp = 0;
    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            uint8_t d = fonts[fontp + x / 8];
            uint8_t linep = (y / 8) * 32 + x;
            if (d & (0x80 >> (x % 8)))
                line[linep] = line[linep] + (1 << mask);
        }
        mask--;
        if (mask < 0)
            mask = 7;
        fontp += (w + 7) / 8;
    }

    if (inverse) {
        for (y = 0; y < (h / 8); y++) {
            for (x = 0; x < w; x++) {
                line[y * 32 + x] = RotateByte(line[y * 32 + x]);
            }
        }
    }
}

void UnderlineBitmap(uint8_t *line, uint8_t w, uint8_t h) {
    int x, y;
    uint8_t wk;
    for (y = 0; y < (h / 8); y++) {
        for (x = 0; x < w; x++) {
            wk = line[y * 32 + x];
            if ((y + 1) == (h / 8))
                line[y * 32 + x] = wk + 0x80;
        }
    }
}

void ReversBitmap(uint8_t *line, uint8_t w, uint8_t h) {
    int x, y;
    uint8_t wk;
    for (y = 0; y < (h / 8); y++) {
        for (x = 0; x < w; x++) {
            wk = line[y * 32 + x];
            line[y * 32 + x] = ~wk;
        }
    }
}

void ShowFont(uint8_t *fonts, uint8_t pw, uint8_t ph) {
    int x, y, fpos;
    printf("[ShowFont pw=%d ph=%d]\n", pw, ph);
    fpos = 0;
    for (y = 0; y < ph; y++) {
        printf("%02d", y);
        for (x = 0; x < pw; x++) {
            if (fonts[fpos + x / 8] & (0x80 >> (x % 8))) {
                printf("*");
            } else {
                printf(".");
            }
        }
        printf("\n");
        fpos = fpos + (pw + 7) / 8;
    }
    printf("\n");
}

void ShowBitmap(uint8_t *bitmap, uint8_t pw, uint8_t ph) {
    int x, y, fpos;
    printf("[ShowBitmap pw=%d ph=%d]\n", pw, ph);

    fpos = 0;
    for (y = 0; y < ph; y++) {
        printf("%02d", y);
        for (x = 0; x < pw; x++) {
            if (bitmap[x + (y / 8) * 32] & (0x80 >> fpos)) {
                printf("*");
            } else {
                printf(".");
            }
        }
        printf("\n");
        fpos++;
        if (fpos > 7)
            fpos = 0;
    }
    printf("\n");
}

uint8_t RotateByte(uint8_t ch1) {
    uint8_t ch2 = 0;
    int j;
    for (j = 0; j < 8; j++) {
        ch2 = (ch2 << 1) + (ch1 & 0x01);
        ch1 = ch1 >> 1;
    }
    return ch2;
}

#endif /* CONFIG_ANJAY_CLIENT_LCD */
