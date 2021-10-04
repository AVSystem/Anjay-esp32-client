/*
 * Copyright 2021 AVSystem <avsystem@avsystem.com>
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

#include "utils.h"

#include <string.h>

#include "esp_system.h"

#include <avsystem/commons/avs_utils.h>

int get_device_id(device_id_t *out_id) {
    memset(out_id->value, 0, sizeof(out_id->value));

    uint8_t mac[6];
    if (esp_efuse_mac_get_default(mac) != ESP_OK) {
        return -1;
    }

    return avs_hexlify(out_id->value, sizeof(out_id->value), NULL, mac,
                       sizeof(mac));
}
