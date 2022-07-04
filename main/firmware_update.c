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

#include "firmware_update.h"
#include "cellular_anjay_impl/cellular_event_loop.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#include <avsystem/commons/avs_log.h>

#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "nvs_flash.h"

#include "main.h"

static struct {
    anjay_t *anjay;
    esp_ota_handle_t update_handle;
    const esp_partition_t *update_partition;
    atomic_bool update_requested;
} fw_state;

static int fw_stream_open(void *user_ptr,
                          const char *package_uri,
                          const struct anjay_etag *package_etag) {
    (void) user_ptr;
    (void) package_uri;
    (void) package_etag;

    assert(!fw_state.update_partition);

    fw_state.update_partition = esp_ota_get_next_update_partition(NULL);
    if (!fw_state.update_partition) {
        avs_log(tutorial, ERROR, "Cannot obtain update partition");
        return -1;
    }

    if (esp_ota_begin(fw_state.update_partition, OTA_SIZE_UNKNOWN,
                      &fw_state.update_handle)) {
        avs_log(tutorial, ERROR, "OTA begin failed");
        fw_state.update_partition = NULL;
        return -1;
    }
    return 0;
}

static int fw_stream_write(void *user_ptr, const void *data, size_t length) {
    (void) user_ptr;

    assert(fw_state.update_partition);

    int result = esp_ota_write(fw_state.update_handle, data, length);
    if (result) {
        avs_log(tutorial, ERROR, "OTA write failed");
        return result == ESP_ERR_OTA_VALIDATE_FAILED
                       ? ANJAY_FW_UPDATE_ERR_UNSUPPORTED_PACKAGE_TYPE
                       : -1;
    }
    return 0;
}

static int fw_stream_finish(void *user_ptr) {
    (void) user_ptr;

    assert(fw_state.update_partition);

    int result = esp_ota_end(fw_state.update_handle);
    if (result) {
        avs_log(tutorial, ERROR, "OTA end failed");
        fw_state.update_partition = NULL;
        return result == ESP_ERR_OTA_VALIDATE_FAILED
                       ? ANJAY_FW_UPDATE_ERR_INTEGRITY_FAILURE
                       : -1;
    }
    return 0;
}

static void fw_reset(void *user_ptr) {
    (void) user_ptr;

    if (fw_state.update_partition) {
        esp_ota_abort(fw_state.update_handle);
        fw_state.update_partition = NULL;
    }
}

static int fw_perform_upgrade(void *user_ptr) {
    (void) user_ptr;

    int result = esp_ota_set_boot_partition(fw_state.update_partition);
    if (result) {
        fw_state.update_partition = NULL;
        return result == ESP_ERR_OTA_VALIDATE_FAILED
                       ? ANJAY_FW_UPDATE_ERR_INTEGRITY_FAILURE
                       : -1;
    }

#ifdef CONFIG_ANJAY_CLIENT_CELLULAR_EVENT_LOOP
    if (cellular_event_loop_interrupt()) {
        return -1;
    }
#else
    if (anjay_event_loop_interrupt(fw_state.anjay)) {
        return -1;
    }
#endif // CONFIG_ANJAY_CLIENT_CELLULAR_EVENT_LOOP

    atomic_store(&fw_state.update_requested, true);
    return 0;
}

static const anjay_fw_update_handlers_t HANDLERS = {
    .stream_open = fw_stream_open,
    .stream_write = fw_stream_write,
    .stream_finish = fw_stream_finish,
    .reset = fw_reset,
    .perform_upgrade = fw_perform_upgrade,
};

int fw_update_install(anjay_t *anjay) {
    anjay_fw_update_initial_state_t state = { 0 };
    const esp_partition_t *partition = esp_ota_get_running_partition();
    esp_ota_img_states_t partition_state;
    esp_ota_get_state_partition(partition, &partition_state);

    if (partition_state == ESP_OTA_IMG_UNDEFINED
            || partition_state == ESP_OTA_IMG_PENDING_VERIFY) {
        avs_log(tutorial, INFO, "First boot from partition with new firmware");
        esp_ota_mark_app_valid_cancel_rollback();
        state.result = ANJAY_FW_UPDATE_INITIAL_SUCCESS;
    }

    // make sure this module is installed for single Anjay instance only
    assert(!fw_state.anjay);
    fw_state.anjay = anjay;

    return anjay_fw_update_install(anjay, &HANDLERS, NULL, &state);
}

bool fw_update_requested(void) {
    return atomic_load(&fw_state.update_requested);
}

void fw_update_reboot(void) {
    avs_log(tutorial, INFO, "Rebooting to perform a firmware upgrade...");
    esp_restart();
}
