/*
 * Copyright 2021-2023 AVSystem <avsystem@avsystem.com>
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
 * Copyright (c) 2021-2023 AVSystem
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
#include "lcd.h"
#include "bmpfile.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_vfs.h"
#include "fontx.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

#if CONFIG_ANJAY_CLIENT_LCD

#    if CONFIG_ANJAY_CLIENT_BOARD_M5STICKC_PLUS
#        include "axp192.h"
#        include "st7789.h"

// M5stickC-Plus stuff
#        define CONFIG_WIDTH 135
#        define CONFIG_HEIGHT 240
#        define CONFIG_OFFSETX 52
#        define CONFIG_OFFSETY 40
#        define ANJAY_TEXT_POSITION 200
#        define LWM2M_CLIENT_TEXT_POSITION 220
#        define CONNECTION_STATUS_TEXT_POSITION 30
#        define CONNECTION_STATUS_VALUE_POSITION 45
#        define CONNECTION_STATUS_VALUE_AREA_BEGIN 30
#        define CONNECTION_STATUS_VALUE_AREA_END 60

#    endif /* CONFIG_ANJAY_CLIENT_BOARD_M5STICKC_PLUS */

#    define BUFFPIXEL 20

static TFT_t dev;
static FontxFile fx16G[2];
static FontxFile fx24G[2];
static FontxFile fx32G[2];
static FontxFile fx16M[2];
static FontxFile fx24M[2];
static FontxFile fx32M[2];

static bool spiffs_opened_properly = false;

static const char *connection_status_texts[] = {
    [LCD_CONNECTION_STATUS_DISCONNECTED] = "disconnected",
    [LCD_CONNECTION_STATUS_CONNECTION_ERROR] = "connection error",
    [LCD_CONNECTION_STATUS_CONNECTING] = "connecting",
    [LCD_CONNECTION_STATUS_CONNECTED] = "connected",
    [LCD_CONNECTION_STATUS_WIFI_CONNECTING] = "WiFi Connecting",
    [LCD_CONNECTION_STATUS_WIFI_CONNECTED] = "WiFi Connected",
    [LCD_CONNECTION_STATUS_BG96_SETTING] = "Setting up BG96",
    [LCD_CONNECTION_STATUS_BG96_SET] = "BG96 set up",
    [LCD_CONNECTION_STATUS_UNKNOWN] = "unknown"
};

static void open_SPIFFS_directory(char *path) {
    DIR *dir = opendir(path);
    assert(dir != NULL);
    struct dirent *pe;
    do {
        pe = readdir(dir);
    } while (pe);
    closedir(dir);
}

static void writeText(TFT_t *dev, FontxFile *fx, const char *text, int y) {
    if (spiffs_opened_properly) {
        // get font width & height
        uint8_t buffer[FontxGlyphBufSize];
        uint8_t fontWidth;
        uint8_t fontHeight;
        GetFontx(fx, 0, buffer, &fontWidth, &fontHeight);
        // calculate caption begin to place it in center
        uint8_t width = fontWidth * (strlen(text) - 1);
        uint8_t x = (CONFIG_WIDTH - width) / 2;

        uint16_t color = WHITE;
        lcdDrawString(dev, fx, x, y, text, color);
    }
}

void lcd_write_connection_status(lcd_connection_status_t status) {
    static lcd_connection_status_t status_prev =
            LCD_CONNECTION_STATUS_DISCONNECTED;
    if (status != status_prev) {
        lcdDrawFillRect(&dev, 0, CONNECTION_STATUS_VALUE_AREA_BEGIN,
                        CONFIG_WIDTH, CONNECTION_STATUS_VALUE_AREA_END, BLACK);
        if (status < LCD_CONNECTION_STATUS_END_) {
            writeText(&dev, fx16G, connection_status_texts[status],
                      CONNECTION_STATUS_VALUE_POSITION);
        } else {
            writeText(&dev, fx16G,
                      connection_status_texts[LCD_CONNECTION_STATUS_UNKNOWN],
                      CONNECTION_STATUS_VALUE_POSITION);
        }
        status_prev = status;
    }
}

static void draw_bmp_file(TFT_t *dev, char *file, int width, int height) {
    lcdFillScreen(dev, BLACK);

    // open requested file
    esp_err_t ret;
    FILE *fp = fopen(file, "rb");
    if (fp == NULL) {
        ESP_LOGW(__FUNCTION__, "File not found [%s]", file);
        return;
    }

    // read bmp header
    bmpfile_t *bmp_file = (bmpfile_t *) malloc(sizeof(bmpfile_t));
    ret = fread(bmp_file, 1, sizeof(bmpfile_t), fp);
    assert(ret == sizeof(bmpfile_t));

    if (bmp_file->header.magic[0] != 'B' || bmp_file->header.magic[1] != 'M') {
        ESP_LOGW(__FUNCTION__, "File is not BMP");
        free(bmp_file);
        fclose(fp);
        return;
    }

    if ((bmp_file->dib.depth == 24) && (bmp_file->dib.compress_type == 0)) {
        // BMP rows are padded (if needed) to 4-byte boundary
        uint32_t rowSize = (bmp_file->dib.width * 3 + 3) & ~3;
        int w = bmp_file->dib.width;
        int h = bmp_file->dib.height;
        int x;
        int size;
        int cols;
        int cole;
        int y;
        int rows;
        int rowe;

        if (width >= w) {
            x = (width - w) / 2;
            size = w;
            cols = 0;
            cole = w - 1;
        } else {
            x = 0;
            size = width;
            cols = (w - width) / 2;
            cole = cols + width - 1;
        }

        if (height >= h) {
            y = (height - h) / 2;
            rows = 0;
            rowe = h - 1;
        } else {
            y = 0;
            rows = (h - height) / 2;
            rowe = rows + height - 1;
        }

        uint8_t sdbuffer[3 * BUFFPIXEL]; // pixel buffer (R+G+B per pixel)
        uint16_t *colors = (uint16_t *) malloc(sizeof(uint16_t) * w);

        for (int row = 0; row < h; row++) { // For each scanline...
            if (row < rows || row > rowe)
                continue;
            // Seek to start of scan line.	It might seem labor-
            // intensive to be doing this on every line, but this
            // method covers a lot of gritty details like cropping
            // and scanline padding.  Also, the seek only takes
            // place if the file position actually needs to change
            // (avoids a lot of cluster math in SD library).
            // Bitmap is stored bottom-to-top order (normal BMP)
            int pos = bmp_file->header.offset + (h - 1 - row) * rowSize;
            fseek(fp, pos, SEEK_SET);
            int buffidx = sizeof(sdbuffer); // Force buffer reload

            int index = 0;
            for (int col = 0; col < w; col++) {
                if (buffidx >= sizeof(sdbuffer)) {
                    fread(sdbuffer, sizeof(sdbuffer), 1, fp);
                    buffidx = 0; // Set index to beginning
                }
                if (col < cols || col > cole)
                    continue;
                // Convert pixel from BMP to TFT format, push to display
                uint8_t b = sdbuffer[buffidx++];
                uint8_t g = sdbuffer[buffidx++];
                uint8_t r = sdbuffer[buffidx++];
                colors[index++] = rgb565_conv(r, g, b);
            }
            lcdDrawMultiPixels(dev, x, y, size, colors);
            y++;
        }
        free(colors);
    }
    free(bmp_file);
    fclose(fp);
}

void lcd_init(void) {
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 10,
        .format_if_mount_failed = true
    };

    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGW(__FUNCTION__, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGW(__FUNCTION__, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGW(__FUNCTION__, "Failed to initialize SPIFFS (%s)",
                     esp_err_to_name(ret));
        }
        spiffs_opened_properly = false;
        return;
    } else {
        spiffs_opened_properly = true;
        size_t total = 0, used = 0;
        ret = esp_spiffs_info(NULL, &total, &used);
        if (ret != ESP_OK) {
            ESP_LOGW(__FUNCTION__,
                     "Failed to get SPIFFS partition information (%s)",
                     esp_err_to_name(ret));
        } else {
            ESP_LOGW(__FUNCTION__, "Partition size: total: %d, used: %d", total,
                     used);
        }

        open_SPIFFS_directory("/spiffs/");

        InitFontx(fx16G, "/spiffs/ILGH16XB.FNT", ""); // 8x16Dot Gothic
        InitFontx(fx24G, "/spiffs/ILGH24XB.FNT", ""); // 12x24Dot Gothic
        InitFontx(fx32G, "/spiffs/ILGH32XB.FNT", ""); // 16x32Dot Gothic
        InitFontx(fx16M, "/spiffs/ILMH16XB.FNT", ""); // 8x16Dot Mincyo
        InitFontx(fx24M, "/spiffs/ILMH24XB.FNT", ""); // 12x24Dot Mincyo
        InitFontx(fx32M, "/spiffs/ILMH32XB.FNT", ""); // 16x32Dot Mincyo

        if (!AXP192_PowerOn()
                && !lcdInit(&dev, CONFIG_WIDTH, CONFIG_HEIGHT, CONFIG_OFFSETX,
                            CONFIG_OFFSETY)) {

            lcdFillScreen(&dev, BLACK);
            draw_bmp_file(&dev, "/spiffs/AVSystem.bmp", CONFIG_WIDTH,
                          CONFIG_HEIGHT);

            writeText(&dev, fx24G, "anjay", ANJAY_TEXT_POSITION);
            writeText(&dev, fx16G, "LwM2M Client", LWM2M_CLIENT_TEXT_POSITION);
            writeText(&dev, fx16G,
                      "connection status:", CONNECTION_STATUS_TEXT_POSITION);
            lcd_write_connection_status(LCD_CONNECTION_STATUS_DISCONNECTED);
        }
    }
}

#endif /* CONFIG_ANJAY_CLIENT_LCD */
