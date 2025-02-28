/*
 * Copyright 2021-2025 AVSystem <avsystem@avsystem.com>
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

#include <stdint.h>

#include <freertos/FreeRTOS.h>

#include <esp_err.h>

#include <driver/i2c.h>

#include "i2c_wrapper.h"

#define I2C_TIMEOUT_MS (100)
#define I2C_TIMEOUT_TICKS (I2C_TIMEOUT_MS / portTICK_PERIOD_MS)

int i2c_master_read_slave_reg(const i2c_device_t *const device,
                              const uint8_t i2c_reg,
                              uint8_t *const data_rd,
                              const uint8_t size) {
    if (size == 0) {
        return 0;
    }
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (cmd != NULL) {
        if (i2c_master_start(cmd)) {
            return -1;
        }
        if (i2c_master_write_byte(cmd, (device->address << 1),
                                  I2C_ACK_CHECK_EN)) {
            return -1;
        }
        if (i2c_master_write_byte(cmd, i2c_reg, I2C_ACK_CHECK_EN)) {
            return -1;
        }
        if (i2c_master_start(cmd)) {
            return -1;
        }
        if (i2c_master_write_byte(cmd,
                                  (device->address << 1) | I2C_MASTER_READ,
                                  I2C_ACK_CHECK_EN)) {
            return -1;
        }
        if (size > 1) {
            if (i2c_master_read(cmd, data_rd, size - 1, I2C_MASTER_ACK)) {
                return -1;
            }
        }
        if (i2c_master_read_byte(cmd, data_rd + size - 1, I2C_MASTER_NACK)) {
            return -1;
        }
        if (i2c_master_stop(cmd)) {
            return -1;
        }
        esp_err_t ret =
                i2c_master_cmd_begin(device->port, cmd, I2C_TIMEOUT_TICKS);
        i2c_cmd_link_delete(cmd);
        return (int) ret;
    } else {
        return -1;
    }
}

int i2c_master_write_slave_reg(const i2c_device_t *const device,
                               const uint8_t i2c_reg,
                               const uint8_t data_wr) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (cmd != NULL) {
        if (i2c_master_start(cmd)) {
            return -1;
        }
        if (i2c_master_write_byte(cmd,
                                  (device->address << 1) | I2C_MASTER_WRITE,
                                  I2C_ACK_CHECK_EN)) {
            return -1;
        }
        if (i2c_master_write_byte(cmd, i2c_reg, I2C_ACK_CHECK_EN)) {
            return -1;
        }
        if (i2c_master_write(cmd, &data_wr, 1, I2C_ACK_CHECK_EN)) {
            return -1;
        }
        if (i2c_master_stop(cmd)) {
            return -1;
        }
        esp_err_t ret =
                i2c_master_cmd_begin(device->port, cmd, I2C_TIMEOUT_TICKS);
        i2c_cmd_link_delete(cmd);
        return (int) ret;
    } else {
        return -1;
    }
}

int i2c_device_init(const i2c_device_t *const device) {
    if (i2c_param_config(device->port, &(device->config))) {
        return -1;
    }

    if (i2c_driver_install(device->port, device->config.mode, 0, 0, 0)) {
        return -1;
    }
    return 0;
}
