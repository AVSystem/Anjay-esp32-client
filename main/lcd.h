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
#ifndef _LCD_H_
#define _LCD_H_

#include "sdkconfig.h"
#include <stdint.h>

#if CONFIG_ANJAY_CLIENT_LCD

typedef enum {
    LCD_CONNECTION_STATUS_DISCONNECTED = 0,
    LCD_CONNECTION_STATUS_CONNECTION_ERROR,
    LCD_CONNECTION_STATUS_CONNECTING,
    LCD_CONNECTION_STATUS_CONNECTED,
    LCD_CONNECTION_STATUS_WIFI_CONNECTING,
    LCD_CONNECTION_STATUS_WIFI_CONNECTED,
    LCD_CONNECTION_STATUS_BG96_SETTING,
    LCD_CONNECTION_STATUS_BG96_SET,
    LCD_CONNECTION_STATUS_UNKNOWN,

    LCD_CONNECTION_STATUS_END_
} lcd_connection_status_t;

void lcd_init(void);
void lcd_write_connection_status(lcd_connection_status_t status);

#endif /* CONFIG_ANJAY_CLIENT_LCD */
#endif /* _LCD_H_ */
