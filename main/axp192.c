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
 * Copyright (c) 2021-2024 AVSystem
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
 * AXP192 Datasheet:
 * http://www.x-powers.com/en.php/Info/down/id/50
 */
#include <stdint.h>

#include <driver/i2c.h>

#include "axp192.h"
#include "i2c_wrapper.h"
#include "sdkconfig.h"

#if CONFIG_ANJAY_CLIENT_BOARD_M5STICKC_PLUS

#    define I2C_AXP192_ADDRESS 0x34
#    define I2C_SDA_AXP192 21
#    define I2C_SCL_AXP192 22

static i2c_device_t axp192_device = {
    .config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA_AXP192,
        .scl_io_num = I2C_SCL_AXP192,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 1000000
    },
    .port = I2C_NUM_1,
    .address = I2C_AXP192_ADDRESS
};

int AXP192_PowerOn() {
    if (i2c_device_init(&axp192_device)) {
        return -1;
    }

    // Set LDO2 & LDO3(TFT_LED & TFT) 3.0V
    if (i2c_master_write_slave_reg(&axp192_device, 0x28, 0xcc)) {
        return -1;
    }

    uint8_t data;
    if (i2c_master_read_slave_reg(&axp192_device, 0x28, &data, 1)) {
        return -1;
    }

    // Set ADC sample rate to 200hz
    if (i2c_master_write_slave_reg(&axp192_device, 0x84, 0xF2)) {
        return -1;
    }

    // Set ADC to All Enable
    if (i2c_master_write_slave_reg(&axp192_device, 0x82, 0xff)) {
        return -1;
    }

    // Bat charge voltage to 4.2, Current 100MA
    if (i2c_master_write_slave_reg(&axp192_device, 0x33, 0xc0)) {
        return -1;
    }

    // Depending on configuration enable LDO2, LDO3, DCDC1, DCDC3.
    if (i2c_master_read_slave_reg(&axp192_device, 0x12, &data, 1)) {
        return -1;
    }
    data = (data & 0xEF) | 0x4D;
    if (i2c_master_write_slave_reg(&axp192_device, 0x12, data)) {
        return -1;
    }

    // 128ms power on, 4s power off
    if (i2c_master_write_slave_reg(&axp192_device, 0x36, 0x0C)) {
        return -1;
    }

    // Set RTC voltage to 3.3V
    if (i2c_master_write_slave_reg(&axp192_device, 0x91, 0xF0)) {
        return -1;
    }

    // Set GPIO0 to LDO
    if (i2c_master_write_slave_reg(&axp192_device, 0x90, 0x02)) {
        return -1;
    }

    // Disable vbus hold limit
    if (i2c_master_write_slave_reg(&axp192_device, 0x30, 0x80)) {
        return -1;
    }

    // Set temperature protection
    if (i2c_master_write_slave_reg(&axp192_device, 0x39, 0xfc)) {
        return -1;
    }

    // Enable RTC BAT charge
    if (i2c_master_write_slave_reg(&axp192_device, 0x35, 0xa2)) {
        return -1;
    }

    // Enable bat detection
    if (i2c_master_write_slave_reg(&axp192_device, 0x32, 0x46)) {
        return -1;
    }

    // Set Power off voltage 3.0v
    if (i2c_master_read_slave_reg(&axp192_device, 0x31, &data, 1)) {
        return -1;
    }
    data = (data & 0xF8) | (1 << 2);
    if (i2c_master_write_slave_reg(&axp192_device, 0x31, data)) {
        return -1;
    }
    return 0;
}

int AXP192_SetScreenBrightness(uint8_t brightness) {
    if (brightness > 12) {
        brightness = 12;
    }
    uint8_t buf;
    if (i2c_master_read_slave_reg(&axp192_device, 0x28, &buf, 1)) {
        return -1;
    }
    if (i2c_master_write_slave_reg(&axp192_device, 0x28,
                                   ((buf & 0x0f) | (brightness << 4)))) {
        return -1;
    }
    return 0;
}

int AXP192_EnableCoulombcounter() {
    if (i2c_master_write_slave_reg(&axp192_device, 0xB8, 0x80)) {
        return -1;
    }
    return 0;
}

int AXP192_DisableCoulombcounter() {
    if (i2c_master_write_slave_reg(&axp192_device, 0xB8, 0x00)) {
        return -1;
    }
    return 0;
}

int AXP192_StopCoulombCounter() {
    if (i2c_master_write_slave_reg(&axp192_device, 0xB8, 0xC0)) {
        return -1;
    }
    return 0;
}

int AXP192_ClearCoulombCounter() {
    if (i2c_master_write_slave_reg(&axp192_device, 0xB8, 0xA0)) {
        return -1;
    }
    return 0;
}

#endif /* CONFIG_ANJAY_CLIENT_BOARD_M5STICKC_PLUS */
