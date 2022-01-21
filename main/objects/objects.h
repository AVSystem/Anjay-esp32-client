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

#pragma once

#include <anjay/dm.h>

typedef struct three_axis_sensor_data_struct {
    double x_value;
    double y_value;
    double z_value;
} three_axis_sensor_data_t;

const anjay_dm_object_def_t **push_button_object_create(void);
void push_button_object_release(const anjay_dm_object_def_t **def);
void push_button_object_update(anjay_t *anjay,
                               const anjay_dm_object_def_t *const *def);

const anjay_dm_object_def_t **light_control_object_create(void);
void light_control_object_release(const anjay_dm_object_def_t **def);

const anjay_dm_object_def_t **device_object_create(void);
void device_object_release(const anjay_dm_object_def_t **def);
void device_object_update(anjay_t *anjay,
                          const anjay_dm_object_def_t *const *def);

void sensors_install(anjay_t *anjay);
void sensors_update(anjay_t *anjay);
void sensors_release(void);
void sensors_read_data(void);
