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

#ifndef _CONNECT_H_
#define _CONNECT_H_

#include "esp_log.h"
#include "esp_wifi.h"

void wifi_initialize(void);
esp_err_t wifi_connect(wifi_config_t *conf);
esp_err_t wifi_disconnect(void);
esp_err_t wifi_deinitialize(void);

#endif // _CONNECT_H_
