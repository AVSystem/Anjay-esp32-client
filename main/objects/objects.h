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

#pragma once

#include <stdbool.h>

#include <esp_wifi.h>

#include <anjay/anjay.h>

typedef struct three_axis_sensor_data_struct {
    double x_value;
    double y_value;
    double z_value;
} three_axis_sensor_data_t;

typedef enum {
    ANJAY_WIFI_OBJ_WRITABLE_INSTANCE,
    ANJAY_WIFI_OBJ_PRECONFIGURED_INSTANCE
} wifi_instance_t;

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

const anjay_dm_object_def_t **wlan_object_create(void);
void wlan_object_release(const anjay_dm_object_def_t **def);
void wlan_object_set_instance_wifi_config(
        const anjay_t *anjay,
        const anjay_dm_object_def_t *const *obj_ptr,
        wifi_instance_t iid,
        wifi_config_t *conf);
wifi_config_t wlan_object_get_instance_wifi_config(
        const anjay_dm_object_def_t *const *obj_ptr, wifi_instance_t iid);
void wlan_object_set_instance_enable(
        const anjay_t *anjay,
        const anjay_dm_object_def_t *const *obj_ptr,
        wifi_instance_t iid,
        bool en);
bool wlan_object_is_instance_enabled(
        const anjay_dm_object_def_t *const *obj_ptr, wifi_instance_t iid);
void wlan_object_set_writable_iface_failed(
        const anjay_t *anjay,
        const anjay_dm_object_def_t *const *obj_ptr,
        bool val);

void sensors_install(anjay_t *anjay);
void sensors_update(anjay_t *anjay);
void sensors_release(void);
void sensors_read_data(void);
