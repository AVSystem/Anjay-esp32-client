/*
 * Copyright 2021-2024 AVSystem <avsystem@avsystem.com>
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
 * Copyright (c) 2021 AVSystem
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
#ifndef _ST7789_H_
#define _ST7789_H_

/*
 * ST7789V2 Datasheet:
 * https://ap.zzjf110.com/attachment/file/ST7789V2_SPEC_V1.0.pdf
 */
#include <stdint.h>

#include <driver/spi_master.h>

#include "fontx.h"
#include "sdkconfig.h"

#ifdef CONFIG_ANJAY_CLIENT_LCD

#    define RED 0xf800
#    define GREEN 0x07e0
#    define BLUE 0x001f
#    define BLACK 0x0000
#    define WHITE 0xffff
#    define GRAY 0x8c51
#    define YELLOW 0xFFE0
#    define CYAN 0x07FF
#    define PURPLE 0xF81F

#    define DIRECTION0 0
#    define DIRECTION90 1
#    define DIRECTION180 2
#    define DIRECTION270 3

typedef struct {
    uint16_t _width;
    uint16_t _height;
    uint16_t _offsetx;
    uint16_t _offsety;
    uint16_t _font_direction;
    uint16_t _font_fill;
    uint16_t _font_fill_color;
    uint16_t _font_underline;
    uint16_t _font_underline_color;
    int16_t _dc;
    int16_t _bl;
    spi_device_handle_t _SPIHandle;
} TFT_t;

int lcdInit(TFT_t *dev, int width, int height, int offsetx, int offsety);
void lcdDrawPixel(TFT_t *dev, uint16_t x, uint16_t y, uint16_t color);
void lcdDrawMultiPixels(
        TFT_t *dev, uint16_t x, uint16_t y, uint16_t size, uint16_t *colors);
void lcdDrawFillRect(TFT_t *dev,
                     uint16_t x1,
                     uint16_t y1,
                     uint16_t x2,
                     uint16_t y2,
                     uint16_t color);
void lcdDisplayOff(TFT_t *dev);
void lcdDisplayOn(TFT_t *dev);
void lcdFillScreen(TFT_t *dev, uint16_t color);
void lcdDrawLine(TFT_t *dev,
                 uint16_t x1,
                 uint16_t y1,
                 uint16_t x2,
                 uint16_t y2,
                 uint16_t color);
void lcdDrawRect(TFT_t *dev,
                 uint16_t x1,
                 uint16_t y1,
                 uint16_t x2,
                 uint16_t y2,
                 uint16_t color);
void lcdDrawRectAngle(TFT_t *dev,
                      uint16_t xc,
                      uint16_t yc,
                      uint16_t w,
                      uint16_t h,
                      uint16_t angle,
                      uint16_t color);
void lcdDrawTriangle(TFT_t *dev,
                     uint16_t xc,
                     uint16_t yc,
                     uint16_t w,
                     uint16_t h,
                     uint16_t angle,
                     uint16_t color);
void lcdDrawCircle(
        TFT_t *dev, uint16_t x0, uint16_t y0, uint16_t r, uint16_t color);
void lcdDrawFillCircle(
        TFT_t *dev, uint16_t x0, uint16_t y0, uint16_t r, uint16_t color);
void lcdDrawRoundRect(TFT_t *dev,
                      uint16_t x1,
                      uint16_t y1,
                      uint16_t x2,
                      uint16_t y2,
                      uint16_t r,
                      uint16_t color);
void lcdDrawArrow(TFT_t *dev,
                  uint16_t x0,
                  uint16_t y0,
                  uint16_t x1,
                  uint16_t y1,
                  uint16_t w,
                  uint16_t color);
void lcdDrawFillArrow(TFT_t *dev,
                      uint16_t x0,
                      uint16_t y0,
                      uint16_t x1,
                      uint16_t y1,
                      uint16_t w,
                      uint16_t color);
uint16_t rgb565_conv(uint16_t r, uint16_t g, uint16_t b);
int lcdDrawChar(TFT_t *dev,
                FontxFile *fx,
                uint16_t x,
                uint16_t y,
                uint8_t ascii,
                uint16_t color);
int lcdDrawString(TFT_t *dev,
                  FontxFile *fx,
                  uint16_t x,
                  uint16_t y,
                  const char *ascii,
                  uint16_t color);
// int lcdDrawSJISChar(TFT_t * dev, FontxFile *fx, uint16_t x, uint16_t y,
// uint16_t sjis, uint16_t color); int lcdDrawUTF8Char(TFT_t * dev, FontxFile
// *fx, uint16_t x, uint16_t y, uint8_t *utf8, uint16_t color); int
// lcdDrawUTF8String(TFT_t * dev, FontxFile *fx, uint16_t x, uint16_t y,
// unsigned char *utfs, uint16_t color);
void lcdSetFontDirection(TFT_t *dev, uint16_t);
void lcdSetFontFill(TFT_t *dev, uint16_t color);
void lcdUnsetFontFill(TFT_t *dev);
void lcdSetFontUnderLine(TFT_t *dev, uint16_t color);
void lcdUnsetFontUnderLine(TFT_t *dev);
void lcdBacklightOff(TFT_t *dev);
void lcdBacklightOn(TFT_t *dev);
void lcdInversionOff(TFT_t *dev);
void lcdInversionOn(TFT_t *dev);

#endif /* CONFIG_ANJAY_CLIENT_LCD */
#endif /* _ST7789_H_ */
