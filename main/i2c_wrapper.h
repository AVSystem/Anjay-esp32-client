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
#ifndef _I2C_WRAPPER_H_
#define _I2C_WRAPPER_H_

#include "driver/i2c.h"

#define I2C_ACK_CHECK_EN (1)
#define I2C_ACK_CHECK_DIS (0)
#define I2C_MASTER_PORT I2C_NUM_0

typedef struct i2c_device_struct {
    i2c_config_t config;
    uint8_t port;
    uint8_t address;
} i2c_device_t;

int i2c_master_read_slave_reg(const i2c_device_t *const device,
                              const uint8_t i2c_reg,
                              uint8_t *const data_rd,
                              const uint8_t size);
int i2c_master_write_slave_reg(const i2c_device_t *const device,
                               const uint8_t i2c_reg,
                               const uint8_t data_wr);
int i2c_device_init(const i2c_device_t *const device);

#endif /* _I2C_WRAPPER_H_ */
