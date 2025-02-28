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
#ifndef _MPU6886_H_
#define _MPU6886_H_

#include "objects.h"
#include "sdkconfig.h"

#if CONFIG_ANJAY_CLIENT_BOARD_M5STICKC_PLUS

#    define GRAVITY_CONSTANT (9.80665)
#    define ACCELEROMETER_RANGE (2.0)
#    define GYROSCOPE_RANGE (500.0)

int accelerometer_read_data(void);
int gyroscope_read_data(void);
int temperature_read_data(void);

void accelerometer_get_data(three_axis_sensor_data_t *sensor_data);
void gyroscope_get_data(three_axis_sensor_data_t *sensor_data);
void temperature_get_data(double *sensor_data);

int mpu6886_device_init(void);
void mpu6886_driver_release(void);

#endif /* CONFIG_ANJAY_CLIENT_BOARD_M5STICKC_PLUS */

#endif /* _MPU6886_H_ */
