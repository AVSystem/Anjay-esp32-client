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

#    define STATUS_DISCONNECTED ((uint8_t) 0)
#    define STATUS_CONNECTION_ERROR ((uint8_t) 1)
#    define STATUS_CONNECTING ((uint8_t) 2)
#    define STATUS_CONNECTED ((uint8_t) 3)
#    define STATUS_WIFI_CONNECTING ((uint8_t) 4)
#    define STATUS_WIFI_CONNECTED ((uint8_t) 5)
#    define STATUS_UNKNOWN ((uint8_t) 6)
#    define STATUS_NUMBER_OF_DEFS ((uint8_t) 7)

void lcd_init(void);
void lcd_write_connection_status(uint8_t status);

#endif /* CONFIG_ANJAY_CLIENT_LCD */
#endif /* _LCD_H_ */
