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

/*
 * ST7789V2 Datasheet:
 * https://ap.zzjf110.com/attachment/file/ST7789V2_SPEC_V1.0.pdf
 */
#include <math.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include <driver/gpio.h>
#include <driver/spi_master.h>

#include "st7789.h"

#if CONFIG_ANJAY_CLIENT_LCD

#    define CONFIG_MOSI_GPIO ((int16_t) 15)
#    define CONFIG_SCLK_GPIO ((int16_t) 13)
#    define CONFIG_CS_GPIO ((int16_t) 5)
#    define CONFIG_DC_GPIO ((int16_t) 23)
#    define CONFIG_RESET_GPIO ((int16_t) 18)
#    define CONFIG_BL_GPIO ((int16_t) -1)

#    define TAG "ST7789"

static const int SPI_Command_Mode = 0;
static const int SPI_Data_Mode = 1;
static const int SPI_Frequency = SPI_MASTER_FREQ_20M;
// static const int SPI_Frequency = SPI_MASTER_FREQ_26M;
// static const int SPI_Frequency = SPI_MASTER_FREQ_40M;
// static const int SPI_Frequency = SPI_MASTER_FREQ_80M;

static void delayMS(int ms) {
    int _ms = ms + (portTICK_PERIOD_MS - 1);
    TickType_t xTicksToDelay = _ms / portTICK_PERIOD_MS;
    ESP_LOGD(TAG, "ms=%d _ms=%d portTICK_PERIOD_MS=%d xTicksToDelay=%d", ms,
             _ms, portTICK_PERIOD_MS, xTicksToDelay);
    vTaskDelay(xTicksToDelay);
}

int spi_master_init(TFT_t *dev,
                    int16_t GPIO_MOSI,
                    int16_t GPIO_SCLK,
                    int16_t GPIO_CS,
                    int16_t GPIO_DC,
                    int16_t GPIO_RESET,
                    int16_t GPIO_BL) {
    if (GPIO_CS >= 0) {
        gpio_pad_select_gpio(GPIO_CS);
        if (ESP_OK != gpio_set_direction(GPIO_CS, GPIO_MODE_OUTPUT)) {
            return -1;
        }
        if (ESP_OK != gpio_set_level(GPIO_CS, 0)) {
            return -1;
        }
    }

    if (GPIO_DC >= 0) {
        gpio_pad_select_gpio(GPIO_DC);
        if (ESP_OK != gpio_set_direction(GPIO_DC, GPIO_MODE_OUTPUT)) {
            return -1;
        }
        if (ESP_OK != gpio_set_level(GPIO_DC, 0)) {
            return -1;
        }
    }

    if (GPIO_RESET >= 0) {
        gpio_pad_select_gpio(GPIO_RESET);
        if (ESP_OK != gpio_set_direction(GPIO_RESET, GPIO_MODE_OUTPUT)) {
            return -1;
        }
        if (ESP_OK != gpio_set_level(GPIO_RESET, 1)) {
            return -1;
        }
        delayMS(50);
        if (ESP_OK != gpio_set_level(GPIO_RESET, 0)) {
            return -1;
        }
        delayMS(50);
        if (ESP_OK != gpio_set_level(GPIO_RESET, 1)) {
            return -1;
        }
        delayMS(50);
    }

    if (GPIO_BL >= 0) {
        gpio_pad_select_gpio(GPIO_BL);
        if (ESP_OK != gpio_set_direction(GPIO_BL, GPIO_MODE_OUTPUT)) {
            return -1;
        }
        if (ESP_OK != gpio_set_level(GPIO_BL, 0)) {
            return -1;
        }
    }

    spi_bus_config_t buscfg = {
        .sclk_io_num = GPIO_SCLK,
        .mosi_io_num = GPIO_MOSI,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1
    };

    if (ESP_OK != spi_bus_initialize(HSPI_HOST, &buscfg, 1)) {
        return -1;
    }

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = SPI_Frequency,
        .queue_size = 7,
        .mode = 2,
        .flags = SPI_DEVICE_NO_DUMMY,
    };

    if (GPIO_CS >= 0) {
        devcfg.spics_io_num = GPIO_CS;
    } else {
        devcfg.spics_io_num = -1;
    }

    spi_device_handle_t handle;
    if (ESP_OK != spi_bus_add_device(HSPI_HOST, &devcfg, &handle)) {
        return -1;
    }
    dev->_dc = GPIO_DC;
    dev->_bl = GPIO_BL;
    dev->_SPIHandle = handle;

    return 0;
}

bool spi_master_write_byte(spi_device_handle_t SPIHandle,
                           const uint8_t *Data,
                           size_t DataLength) {
    spi_transaction_t SPITransaction;

    if (DataLength > 0) {
        memset(&SPITransaction, 0, sizeof(spi_transaction_t));
        SPITransaction.length = DataLength * 8;
        SPITransaction.tx_buffer = Data;
        if (ESP_OK != spi_device_transmit(SPIHandle, &SPITransaction)) {
            return false;
        }
    }

    return true;
}

bool spi_master_write_command(TFT_t *dev, uint8_t cmd) {
    static uint8_t Byte = 0;
    Byte = cmd;
    if (ESP_OK == gpio_set_level(dev->_dc, SPI_Command_Mode)) {
        return spi_master_write_byte(dev->_SPIHandle, &Byte, 1);
    } else {
        return false;
    }
}

bool spi_master_write_data_byte(TFT_t *dev, uint8_t data) {
    static uint8_t Byte = 0;
    Byte = data;
    if (ESP_OK == gpio_set_level(dev->_dc, SPI_Data_Mode)) {
        return spi_master_write_byte(dev->_SPIHandle, &Byte, 1);
    } else {
        return false;
    }
}

bool spi_master_write_data_word(TFT_t *dev, uint16_t data) {
    static uint8_t Byte[2];
    Byte[0] = (data >> 8) & 0xFF;
    Byte[1] = data & 0xFF;
    if (ESP_OK == gpio_set_level(dev->_dc, SPI_Data_Mode)) {
        return spi_master_write_byte(dev->_SPIHandle, Byte, 2);
    } else {
        return false;
    }
}

bool spi_master_write_addr(TFT_t *dev, uint16_t addr1, uint16_t addr2) {
    static uint8_t Byte[4];
    Byte[0] = (addr1 >> 8) & 0xFF;
    Byte[1] = addr1 & 0xFF;
    Byte[2] = (addr2 >> 8) & 0xFF;
    Byte[3] = addr2 & 0xFF;
    if (ESP_OK == gpio_set_level(dev->_dc, SPI_Data_Mode)) {
        return spi_master_write_byte(dev->_SPIHandle, Byte, 4);
    } else {
        return false;
    }
}

bool spi_master_write_color(TFT_t *dev, uint16_t color, uint16_t size) {
    static uint8_t Byte[1024];
    int index = 0;
    for (int i = 0; i < size; i++) {
        Byte[index++] = (color >> 8) & 0xFF;
        Byte[index++] = color & 0xFF;
    }
    if (ESP_OK == gpio_set_level(dev->_dc, SPI_Data_Mode)) {
        return spi_master_write_byte(dev->_SPIHandle, Byte, size * 2);
    } else {
        return false;
    }
}

// Add 202001
bool spi_master_write_colors(TFT_t *dev, uint16_t *colors, uint16_t size) {
    static uint8_t Byte[1024];
    int index = 0;
    for (int i = 0; i < size; i++) {
        Byte[index++] = (colors[i] >> 8) & 0xFF;
        Byte[index++] = colors[i] & 0xFF;
    }
    if (ESP_OK == gpio_set_level(dev->_dc, SPI_Data_Mode)) {
        return spi_master_write_byte(dev->_SPIHandle, Byte, size * 2);
    } else {
        return false;
    }
}

int lcdInit(TFT_t *dev, int width, int height, int offsetx, int offsety) {
    dev->_width = width;
    dev->_height = height;
    dev->_offsetx = offsetx;
    dev->_offsety = offsety;
    dev->_font_direction = DIRECTION0;
    dev->_font_fill = false;
    dev->_font_underline = false;

    spi_master_init(dev, CONFIG_MOSI_GPIO, CONFIG_SCLK_GPIO, CONFIG_CS_GPIO,
                    CONFIG_DC_GPIO, CONFIG_RESET_GPIO, CONFIG_BL_GPIO);

    // Software Reset
    if (!spi_master_write_command(dev, 0x01)) {
        return -1;
    }
    delayMS(150);

    // Sleep Out
    if (!spi_master_write_command(dev, 0x11)) {
        return -1;
    }
    delayMS(255);

    // Interface Pixel Format
    if (!spi_master_write_command(dev, 0x3A)) {
        return -1;
    }
    if (!spi_master_write_data_byte(dev, 0x55)) {
        return -1;
    }
    delayMS(20);

    // Memory Data Access Control
    if (!spi_master_write_command(dev, 0x36)
            && !spi_master_write_data_byte(dev, 0x00)) {
        return -1;
    }

    // Column Address Set
    if (!spi_master_write_command(dev, 0x2A)
            && !spi_master_write_data_byte(dev, 0x00)
            && !spi_master_write_data_byte(dev, 0x00)
            && !spi_master_write_data_byte(dev, 0x00)
            && !spi_master_write_data_byte(dev, 0xF0)) {
        return -1;
    }

    // Row Address Set
    if (!spi_master_write_command(dev, 0x2B)
            && !spi_master_write_data_byte(dev, 0x00)
            && !spi_master_write_data_byte(dev, 0x00)
            && !spi_master_write_data_byte(dev, 0x00)
            && !spi_master_write_data_byte(dev, 0xF0)) {
        return -1;
    }

    // Display Inversion On
    if (!spi_master_write_command(dev, 0x21)) {
        return -1;
    }
    delayMS(10);

    // Normal Display Mode On
    if (!spi_master_write_command(dev, 0x13)) {
        return -1;
    }
    delayMS(10);

    // Display ON
    if (!spi_master_write_command(dev, 0x29)) {
        return -1;
    }
    delayMS(255);

    if (dev->_bl >= 0) {
        gpio_set_level(dev->_bl, 1);
    }

    return 0;
}

// Draw pixel
// x:X coordinate
// y:Y coordinate
// color:color
void lcdDrawPixel(TFT_t *dev, uint16_t x, uint16_t y, uint16_t color) {
    if (x >= dev->_width)
        return;
    if (y >= dev->_height)
        return;

    uint16_t _x = x + dev->_offsetx;
    uint16_t _y = y + dev->_offsety;

    spi_master_write_command(dev, 0x2A); // set column(x) address
    spi_master_write_addr(dev, _x, _x);
    spi_master_write_command(dev, 0x2B); // set Page(y) address
    spi_master_write_addr(dev, _y, _y);
    spi_master_write_command(dev, 0x2C); //	Memory Write
    spi_master_write_data_word(dev, color);
}

// Draw multi pixel
// x:X coordinate
// y:Y coordinate
// size:Number of colors
// colors:colors
void lcdDrawMultiPixels(
        TFT_t *dev, uint16_t x, uint16_t y, uint16_t size, uint16_t *colors) {
    if (x + size > dev->_width)
        return;
    if (y >= dev->_height)
        return;

    uint16_t _x1 = x + dev->_offsetx;
    uint16_t _x2 = _x1 + size;
    uint16_t _y1 = y + dev->_offsety;
    uint16_t _y2 = _y1;

    spi_master_write_command(dev, 0x2A); // set column(x) address
    spi_master_write_addr(dev, _x1, _x2);
    spi_master_write_command(dev, 0x2B); // set Page(y) address
    spi_master_write_addr(dev, _y1, _y2);
    spi_master_write_command(dev, 0x2C); //	Memory Write
    spi_master_write_colors(dev, colors, size);
}

// Draw rectangle of filling
// x1:Start X coordinate
// y1:Start Y coordinate
// x2:End X coordinate
// y2:End Y coordinate
// color:color
void lcdDrawFillRect(TFT_t *dev,
                     uint16_t x1,
                     uint16_t y1,
                     uint16_t x2,
                     uint16_t y2,
                     uint16_t color) {
    if (x1 >= dev->_width)
        return;
    if (x2 >= dev->_width)
        x2 = dev->_width - 1;
    if (y1 >= dev->_height)
        return;
    if (y2 >= dev->_height)
        y2 = dev->_height - 1;

    ESP_LOGD(TAG, "offset(x)=%d offset(y)=%d", dev->_offsetx, dev->_offsety);
    uint16_t _x1 = x1 + dev->_offsetx;
    uint16_t _x2 = x2 + dev->_offsetx;
    uint16_t _y1 = y1 + dev->_offsety;
    uint16_t _y2 = y2 + dev->_offsety;

    spi_master_write_command(dev, 0x2A); // set column(x) address
    spi_master_write_addr(dev, _x1, _x2);
    spi_master_write_command(dev, 0x2B); // set Page(y) address
    spi_master_write_addr(dev, _y1, _y2);
    spi_master_write_command(dev, 0x2C); //	Memory Write
    for (int i = _x1; i <= _x2; i++) {
        uint16_t size = _y2 - _y1 + 1;
        spi_master_write_color(dev, color, size);
    }
}

// Display OFF
void lcdDisplayOff(TFT_t *dev) {
    spi_master_write_command(dev, 0x28);
}

// Display ON
void lcdDisplayOn(TFT_t *dev) {
    spi_master_write_command(dev, 0x29);
}

// Fill screen
// color:color
void lcdFillScreen(TFT_t *dev, uint16_t color) {
    lcdDrawFillRect(dev, 0, 0, dev->_width - 1, dev->_height - 1, color);
}

// Draw line
// x1:Start X coordinate
// y1:Start Y coordinate
// x2:End X coordinate
// y2:End Y coordinate
// color:color
void lcdDrawLine(TFT_t *dev,
                 uint16_t x1,
                 uint16_t y1,
                 uint16_t x2,
                 uint16_t y2,
                 uint16_t color) {
    int i;
    int dx, dy;
    int sx, sy;
    int E;

    /* distance between two points */
    dx = (x2 > x1) ? x2 - x1 : x1 - x2;
    dy = (y2 > y1) ? y2 - y1 : y1 - y2;

    /* direction of two point */
    sx = (x2 > x1) ? 1 : -1;
    sy = (y2 > y1) ? 1 : -1;

    /* inclination < 1 */
    if (dx > dy) {
        E = -dx;
        for (i = 0; i <= dx; i++) {
            lcdDrawPixel(dev, x1, y1, color);
            x1 += sx;
            E += 2 * dy;
            if (E >= 0) {
                y1 += sy;
                E -= 2 * dx;
            }
        }

        /* inclination >= 1 */
    } else {
        E = -dy;
        for (i = 0; i <= dy; i++) {
            lcdDrawPixel(dev, x1, y1, color);
            y1 += sy;
            E += 2 * dx;
            if (E >= 0) {
                x1 += sx;
                E -= 2 * dy;
            }
        }
    }
}

// Draw rectangle
// x1:Start X coordinate
// y1:Start Y coordinate
// x2:End	X coordinate
// y2:End	Y coordinate
// color:color
void lcdDrawRect(TFT_t *dev,
                 uint16_t x1,
                 uint16_t y1,
                 uint16_t x2,
                 uint16_t y2,
                 uint16_t color) {
    lcdDrawLine(dev, x1, y1, x2, y1, color);
    lcdDrawLine(dev, x2, y1, x2, y2, color);
    lcdDrawLine(dev, x2, y2, x1, y2, color);
    lcdDrawLine(dev, x1, y2, x1, y1, color);
}

// Draw rectangle with angle
// xc:Center X coordinate
// yc:Center Y coordinate
// w:Width of rectangle
// h:Height of rectangle
// angle :Angle of rectangle
// color :color

// When the origin is (0, 0), the point (x1, y1) after rotating the point (x, y)
// by the angle is obtained by the following calculation.
// x1 = x * cos(angle) - y * sin(angle)
// y1 = x * sin(angle) + y * cos(angle)
void lcdDrawRectAngle(TFT_t *dev,
                      uint16_t xc,
                      uint16_t yc,
                      uint16_t w,
                      uint16_t h,
                      uint16_t angle,
                      uint16_t color) {
    double xd, yd, rd;
    int x1, y1;
    int x2, y2;
    int x3, y3;
    int x4, y4;
    rd = -angle * M_PI / 180.0;
    xd = 0.0 - w / 2;
    yd = h / 2;
    x1 = (int) (xd * cos(rd) - yd * sin(rd) + xc);
    y1 = (int) (xd * sin(rd) + yd * cos(rd) + yc);

    yd = 0.0 - yd;
    x2 = (int) (xd * cos(rd) - yd * sin(rd) + xc);
    y2 = (int) (xd * sin(rd) + yd * cos(rd) + yc);

    xd = w / 2;
    yd = h / 2;
    x3 = (int) (xd * cos(rd) - yd * sin(rd) + xc);
    y3 = (int) (xd * sin(rd) + yd * cos(rd) + yc);

    yd = 0.0 - yd;
    x4 = (int) (xd * cos(rd) - yd * sin(rd) + xc);
    y4 = (int) (xd * sin(rd) + yd * cos(rd) + yc);

    lcdDrawLine(dev, x1, y1, x2, y2, color);
    lcdDrawLine(dev, x1, y1, x3, y3, color);
    lcdDrawLine(dev, x2, y2, x4, y4, color);
    lcdDrawLine(dev, x3, y3, x4, y4, color);
}

// Draw triangle
// xc:Center X coordinate
// yc:Center Y coordinate
// w:Width of triangle
// h:Height of triangle
// angle :Angle of triangle
// color :color

// When the origin is (0, 0), the point (x1, y1) after rotating the point (x, y)
// by the angle is obtained by the following calculation.
// x1 = x * cos(angle) - y * sin(angle)
// y1 = x * sin(angle) + y * cos(angle)
void lcdDrawTriangle(TFT_t *dev,
                     uint16_t xc,
                     uint16_t yc,
                     uint16_t w,
                     uint16_t h,
                     uint16_t angle,
                     uint16_t color) {
    double xd, yd, rd;
    int x1, y1;
    int x2, y2;
    int x3, y3;
    rd = -angle * M_PI / 180.0;
    xd = 0.0;
    yd = h / 2;
    x1 = (int) (xd * cos(rd) - yd * sin(rd) + xc);
    y1 = (int) (xd * sin(rd) + yd * cos(rd) + yc);

    xd = w / 2;
    yd = 0.0 - yd;
    x2 = (int) (xd * cos(rd) - yd * sin(rd) + xc);
    y2 = (int) (xd * sin(rd) + yd * cos(rd) + yc);

    xd = 0.0 - w / 2;
    x3 = (int) (xd * cos(rd) - yd * sin(rd) + xc);
    y3 = (int) (xd * sin(rd) + yd * cos(rd) + yc);

    lcdDrawLine(dev, x1, y1, x2, y2, color);
    lcdDrawLine(dev, x1, y1, x3, y3, color);
    lcdDrawLine(dev, x2, y2, x3, y3, color);
}

// Draw circle
// x0:Central X coordinate
// y0:Central Y coordinate
// r:radius
// color:color
void lcdDrawCircle(
        TFT_t *dev, uint16_t x0, uint16_t y0, uint16_t r, uint16_t color) {
    int x;
    int y;
    int err;
    int old_err;

    x = 0;
    y = -r;
    err = 2 - 2 * r;
    do {
        lcdDrawPixel(dev, x0 - x, y0 + y, color);
        lcdDrawPixel(dev, x0 - y, y0 - x, color);
        lcdDrawPixel(dev, x0 + x, y0 - y, color);
        lcdDrawPixel(dev, x0 + y, y0 + x, color);
        if ((old_err = err) <= x)
            err += ++x * 2 + 1;
        if (old_err > y || err > x)
            err += ++y * 2 + 1;
    } while (y < 0);
}

// Draw circle of filling
// x0:Central X coordinate
// y0:Central Y coordinate
// r:radius
// color:color
void lcdDrawFillCircle(
        TFT_t *dev, uint16_t x0, uint16_t y0, uint16_t r, uint16_t color) {
    int x;
    int y;
    int err;
    int old_err;
    int ChangeX;

    x = 0;
    y = -r;
    err = 2 - 2 * r;
    ChangeX = 1;
    do {
        if (ChangeX) {
            lcdDrawLine(dev, x0 - x, y0 - y, x0 - x, y0 + y, color);
            lcdDrawLine(dev, x0 + x, y0 - y, x0 + x, y0 + y, color);
        } // endif
        ChangeX = (old_err = err) <= x;
        if (ChangeX)
            err += ++x * 2 + 1;
        if (old_err > y || err > x)
            err += ++y * 2 + 1;
    } while (y <= 0);
}

// Draw rectangle with round corner
// x1:Start X coordinate
// y1:Start Y coordinate
// x2:End	X coordinate
// y2:End	Y coordinate
// r:radius
// color:color
void lcdDrawRoundRect(TFT_t *dev,
                      uint16_t x1,
                      uint16_t y1,
                      uint16_t x2,
                      uint16_t y2,
                      uint16_t r,
                      uint16_t color) {
    int x;
    int y;
    int err;
    int old_err;
    unsigned char temp;

    if (x1 > x2) {
        temp = x1;
        x1 = x2;
        x2 = temp;
    } // endif

    if (y1 > y2) {
        temp = y1;
        y1 = y2;
        y2 = temp;
    } // endif

    ESP_LOGD(TAG, "x1=%d x2=%d delta=%d r=%d", x1, x2, x2 - x1, r);
    ESP_LOGD(TAG, "y1=%d y2=%d delta=%d r=%d", y1, y2, y2 - y1, r);
    if (x2 - x1 < r)
        return; // Add 20190517
    if (y2 - y1 < r)
        return; // Add 20190517

    x = 0;
    y = -r;
    err = 2 - 2 * r;

    do {
        if (x) {
            lcdDrawPixel(dev, x1 + r - x, y1 + r + y, color);
            lcdDrawPixel(dev, x2 - r + x, y1 + r + y, color);
            lcdDrawPixel(dev, x1 + r - x, y2 - r - y, color);
            lcdDrawPixel(dev, x2 - r + x, y2 - r - y, color);
        } // endif
        if ((old_err = err) <= x)
            err += ++x * 2 + 1;
        if (old_err > y || err > x)
            err += ++y * 2 + 1;
    } while (y < 0);

    ESP_LOGD(TAG, "x1+r=%d x2-r=%d", x1 + r, x2 - r);
    lcdDrawLine(dev, x1 + r, y1, x2 - r, y1, color);
    lcdDrawLine(dev, x1 + r, y2, x2 - r, y2, color);
    ESP_LOGD(TAG, "y1+r=%d y2-r=%d", y1 + r, y2 - r);
    lcdDrawLine(dev, x1, y1 + r, x1, y2 - r, color);
    lcdDrawLine(dev, x2, y1 + r, x2, y2 - r, color);
}

// Draw arrow
// x1:Start X coordinate
// y1:Start Y coordinate
// x2:End	X coordinate
// y2:End	Y coordinate
// w:Width of the botom
// color:color
// Thanks http://k-hiura.cocolog-nifty.com/blog/2010/11/post-2a62.html
void lcdDrawArrow(TFT_t *dev,
                  uint16_t x0,
                  uint16_t y0,
                  uint16_t x1,
                  uint16_t y1,
                  uint16_t w,
                  uint16_t color) {
    double Vx = x1 - x0;
    double Vy = y1 - y0;
    double v = sqrt(Vx * Vx + Vy * Vy);
    //	 printf("v=%f\n",v);
    double Ux = Vx / v;
    double Uy = Vy / v;

    uint16_t L[2], R[2];
    L[0] = x1 - Uy * w - Ux * v;
    L[1] = y1 + Ux * w - Uy * v;
    R[0] = x1 + Uy * w - Ux * v;
    R[1] = y1 - Ux * w - Uy * v;
    // printf("L=%d-%d R=%d-%d\n",L[0],L[1],R[0],R[1]);

    // lcdDrawLine(x0,y0,x1,y1,color);
    lcdDrawLine(dev, x1, y1, L[0], L[1], color);
    lcdDrawLine(dev, x1, y1, R[0], R[1], color);
    lcdDrawLine(dev, L[0], L[1], R[0], R[1], color);
}

// Draw arrow of filling
// x1:Start X coordinate
// y1:Start Y coordinate
// x2:End	X coordinate
// y2:End	Y coordinate
// w:Width of the botom
// color:color
void lcdDrawFillArrow(TFT_t *dev,
                      uint16_t x0,
                      uint16_t y0,
                      uint16_t x1,
                      uint16_t y1,
                      uint16_t w,
                      uint16_t color) {
    double Vx = x1 - x0;
    double Vy = y1 - y0;
    double v = sqrt(Vx * Vx + Vy * Vy);
    // printf("v=%f\n",v);
    double Ux = Vx / v;
    double Uy = Vy / v;

    uint16_t L[2], R[2];
    L[0] = x1 - Uy * w - Ux * v;
    L[1] = y1 + Ux * w - Uy * v;
    R[0] = x1 + Uy * w - Ux * v;
    R[1] = y1 - Ux * w - Uy * v;
    // printf("L=%d-%d R=%d-%d\n",L[0],L[1],R[0],R[1]);

    lcdDrawLine(dev, x0, y0, x1, y1, color);
    lcdDrawLine(dev, x1, y1, L[0], L[1], color);
    lcdDrawLine(dev, x1, y1, R[0], R[1], color);
    lcdDrawLine(dev, L[0], L[1], R[0], R[1], color);

    int ww;
    for (ww = w - 1; ww > 0; ww--) {
        L[0] = x1 - Uy * ww - Ux * v;
        L[1] = y1 + Ux * ww - Uy * v;
        R[0] = x1 + Uy * ww - Ux * v;
        R[1] = y1 - Ux * ww - Uy * v;
        // printf("Fill>L=%d-%d R=%d-%d\n",L[0],L[1],R[0],R[1]);
        lcdDrawLine(dev, x1, y1, L[0], L[1], color);
        lcdDrawLine(dev, x1, y1, R[0], R[1], color);
    }
}

// RGB565 conversion
// RGB565 is R(5)+G(6)+B(5)=16bit color format.
// Bit image "RRRRRGGGGGGBBBBB"
uint16_t rgb565_conv(uint16_t r, uint16_t g, uint16_t b) {
    return (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

// Draw ASCII character
// x:X coordinate
// y:Y coordinate
// ascii: ascii code
// color:color
int lcdDrawChar(TFT_t *dev,
                FontxFile *fxs,
                uint16_t x,
                uint16_t y,
                uint8_t ascii,
                uint16_t color) {
    uint16_t xx, yy, bit, ofs;
    unsigned char fonts[128]; // font pattern
    unsigned char pw, ph;
    int h, w;
    uint16_t mask;
    bool rc;

    rc = GetFontx(fxs, ascii, fonts, &pw, &ph);
    if (!rc)
        return 0;

    int16_t xd1 = 0;
    int16_t yd1 = 0;
    int16_t xd2 = 0;
    int16_t yd2 = 0;
    uint16_t xss = 0;
    uint16_t yss = 0;
    int16_t xsd = 0;
    int16_t ysd = 0;
    int16_t next = 0;
    uint16_t x0 = 0;
    uint16_t x1 = 0;
    uint16_t y0 = 0;
    uint16_t y1 = 0;
    if (dev->_font_direction == 0) {
        xd1 = +1;
        yd1 = +1; //-1;
        xd2 = 0;
        yd2 = 0;
        xss = x;
        yss = y - (ph - 1);
        xsd = 1;
        ysd = 0;
        next = x + pw;

        x0 = x;
        y0 = y - (ph - 1);
        x1 = x + (pw - 1);
        y1 = y;
    } else if (dev->_font_direction == 2) {
        xd1 = -1;
        yd1 = -1; //+1;
        xd2 = 0;
        yd2 = 0;
        xss = x;
        yss = y + ph + 1;
        xsd = 1;
        ysd = 0;
        next = x - pw;

        x0 = x - (pw - 1);
        y0 = y;
        x1 = x;
        y1 = y + (ph - 1);
    } else if (dev->_font_direction == 1) {
        xd1 = 0;
        yd1 = 0;
        xd2 = -1;
        yd2 = +1; //-1;
        xss = x + ph;
        yss = y;
        xsd = 0;
        ysd = 1;
        next = y + pw; // y - pw;

        x0 = x;
        y0 = y;
        x1 = x + (ph - 1);
        y1 = y + (pw - 1);
    } else if (dev->_font_direction == 3) {
        xd1 = 0;
        yd1 = 0;
        xd2 = +1;
        yd2 = -1; //+1;
        xss = x - (ph - 1);
        yss = y;
        xsd = 0;
        ysd = 1;
        next = y - pw; // y + pw;

        x0 = x - (ph - 1);
        y0 = y - (pw - 1);
        x1 = x;
        y1 = y;
    }

    if (dev->_font_fill)
        lcdDrawFillRect(dev, x0, y0, x1, y1, dev->_font_fill_color);

    int bits;
    ofs = 0;
    yy = yss;
    xx = xss;
    for (h = 0; h < ph; h++) {
        if (xsd)
            xx = xss;
        if (ysd)
            yy = yss;
        bits = pw;
        for (w = 0; w < ((pw + 4) / 8); w++) {
            mask = 0x80;
            for (bit = 0; bit < 8; bit++) {
                bits--;
                if (bits < 0)
                    continue;
                if (fonts[ofs] & mask) {
                    lcdDrawPixel(dev, xx, yy, color);
                }
                if (h == (ph - 2) && dev->_font_underline)
                    lcdDrawPixel(dev, xx, yy, dev->_font_underline_color);
                if (h == (ph - 1) && dev->_font_underline)
                    lcdDrawPixel(dev, xx, yy, dev->_font_underline_color);
                xx = xx + xd1;
                yy = yy + yd2;
                mask = mask >> 1;
            }
            ofs++;
        }
        yy = yy + yd1;
        xx = xx + xd2;
    }

    if (next < 0)
        next = 0;
    return next;
}

int lcdDrawString(TFT_t *dev,
                  FontxFile *fx,
                  uint16_t x,
                  uint16_t y,
                  const char *ascii,
                  uint16_t color) {
    int length = strlen((char *) ascii);
    for (int i = 0; i < length; i++) {
        if (dev->_font_direction == 0)
            x = lcdDrawChar(dev, fx, x, y, ascii[i], color);
        if (dev->_font_direction == 1)
            y = lcdDrawChar(dev, fx, x, y, ascii[i], color);
        if (dev->_font_direction == 2)
            x = lcdDrawChar(dev, fx, x, y, ascii[i], color);
        if (dev->_font_direction == 3)
            y = lcdDrawChar(dev, fx, x, y, ascii[i], color);
    }
    if (dev->_font_direction == 0)
        return x;
    if (dev->_font_direction == 2)
        return x;
    if (dev->_font_direction == 1)
        return y;
    if (dev->_font_direction == 3)
        return y;
    return 0;
}

// Set font direction
// dir:Direction
void lcdSetFontDirection(TFT_t *dev, uint16_t dir) {
    dev->_font_direction = dir;
}

// Set font filling
// color:fill color
void lcdSetFontFill(TFT_t *dev, uint16_t color) {
    dev->_font_fill = true;
    dev->_font_fill_color = color;
}

// UnSet font filling
void lcdUnsetFontFill(TFT_t *dev) {
    dev->_font_fill = false;
}

// Set font underline
// color:frame color
void lcdSetFontUnderLine(TFT_t *dev, uint16_t color) {
    dev->_font_underline = true;
    dev->_font_underline_color = color;
}

// UnSet font underline
void lcdUnsetFontUnderLine(TFT_t *dev) {
    dev->_font_underline = false;
}

// Backlight OFF
void lcdBacklightOff(TFT_t *dev) {
    if (dev->_bl >= 0) {
        gpio_set_level(dev->_bl, 0);
    }
}

// Backlight ON
void lcdBacklightOn(TFT_t *dev) {
    if (dev->_bl >= 0) {
        gpio_set_level(dev->_bl, 1);
    }
}

// Display Inversion Off
void lcdInversionOff(TFT_t *dev) {
    spi_master_write_command(dev, 0x20); // Display Inversion Off
}

// Display Inversion On
void lcdInversionOn(TFT_t *dev) {
    spi_master_write_command(dev, 0x21); // Display Inversion On
}

#endif /* CONFIG_ANJAY_CLIENT_LCD */
