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

#ifndef _MAIN_H_
#define _MAIN_H_

#define MAIN_NVS_CONFIG_NAMESPACE "config"
#define MAIN_NVS_WRITABLE_WIFI_CONFIG_NAMESPACE "writable_wifi"
#define MAIN_NVS_WIFI_SSID_KEY "wifi_ssid"
#define MAIN_NVS_WIFI_PASSWORD_KEY "wifi_pswd"
#define MAIN_NVS_ENABLE_KEY "wifi_inter_en"

void schedule_change_config(void);

#endif // _MAIN_H_
