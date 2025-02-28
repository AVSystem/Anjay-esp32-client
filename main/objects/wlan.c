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

#include <assert.h>
#include <pthread.h>
#include <stdbool.h>

#include <anjay/anjay.h>
#include <avsystem/commons/avs_defs.h>
#include <avsystem/commons/avs_log.h>
#include <avsystem/commons/avs_memory.h>

#include "nvs_flash.h"

#include "connect.h"
#include "main.h"
#include "objects.h"

/**
 * Wlan connectivity object ID
 */
#define OID_WLAN_CONNECTIVITY 12

/**
 * Interface name: RW, Single, Mandatory
 * type: string, range: N/A, unit: N/A
 * Human-readable identifier eg. wlan0
 */
#define RID_INTERFACE_NAME 0

/**
 * Enable: RW, Single, Mandatory
 * type: boolean, range: N/A, unit: N/A
 * 0: Disabled 1: Enabled Enable / Disable interface When disabled radio
 * must also be disabled
 */
#define RID_ENABLE 1

/**
 * Status: R, Single, Mandatory
 * type: integer, range: N/A, unit: N/A
 * 0: Disabled 1: UP (OK) 2: Error
 */
#define RID_STATUS 3

/**
 * BSSID: R, Single, Mandatory
 * type: string, range: 12 bytes, unit: N/A
 * The MAC address of the interface, in hexadecimal form.
 */
#define RID_BSSID 4

/**
 * SSID: RW, Single, Mandatory
 * type: string, range: 1..32, unit: N/A
 * The Service Set Identifier for this interface.
 */
#define RID_SSID 5

/**
 * Mode: RW, Single, Mandatory
 * type: integer, range: N/A, unit: N/A
 * 0: Access Point 1: Client (Station) 2: Bridge 3: Repeater
 */
#define RID_MODE 8

/**
 * Channel: RW, Single, Mandatory
 * type: integer, range: 0..255, unit: N/A
 * The current radio channel in use by this interface.
 */
#define RID_CHANNEL 9

/**
 * Standard: RW, Single, Mandatory
 * type: integer, range: N/A, unit: N/A
 * 0: 802.11a 1: 802.11b 2: 802.11bg  3: 802.11g 4: 802.11n 5: 802.11bgn
 * 6: 802.11ac 7: 802.11ah
 */
#define RID_STANDARD 14

/**
 * Authentication Mode: RW, Single, Mandatory
 * type: integer, range: N/A, unit: N/A
 * 0: None (Open) 1: PSK 2: EAP 3: EAP+PSK 4: EAPSIM
 */
#define RID_AUTHENTICATION_MODE 15

/**
 * WPA Key Phrase: W, Single, Optional
 * type: string, range: 1..64, unit: N/A
 * WPA/WPA2 Key Phrase. Write Only.
 */
#define RID_WPA_KEY_PHRASE 18

typedef struct wlan_connectivity_instance_struct {
    bool enable;
    bool enable_backup;
    wifi_config_t wifi_config;
    wifi_config_t wifi_config_backup;
} wlan_connectivity_instance_t;

typedef struct wlan_connectivity_object_struct {
    const anjay_dm_object_def_t *def;
    wlan_connectivity_instance_t instances[2];
    bool writable_iface_failed;
} wlan_connectivity_object_t;

static inline wlan_connectivity_object_t *
get_obj(const anjay_dm_object_def_t *const *obj_ptr) {
    assert(obj_ptr);
    return AVS_CONTAINER_OF(obj_ptr, wlan_connectivity_object_t, def);
}

static int list_instances(anjay_t *anjay,
                          const anjay_dm_object_def_t *const *obj_ptr,
                          anjay_dm_list_ctx_t *ctx) {
    (void) anjay;

    wlan_connectivity_object_t *obj = get_obj(obj_ptr);
    for (anjay_iid_t iid = 0; iid < AVS_ARRAY_SIZE(obj->instances); iid++) {
        anjay_dm_emit(ctx, iid);
    }
    return 0;
}

static int instance_reset(anjay_t *anjay,
                          const anjay_dm_object_def_t *const *obj_ptr,
                          anjay_iid_t iid) {
    (void) anjay;
    wlan_connectivity_object_t *obj = get_obj(obj_ptr);
    assert(obj);
    assert(iid < AVS_ARRAY_SIZE(obj->instances));
    if (iid == ANJAY_WIFI_OBJ_WRITABLE_INSTANCE) {
        wlan_connectivity_instance_t *inst = &obj->instances[iid];
        inst->wifi_config.sta.ssid[0] = '\0';
        inst->wifi_config.sta.password[0] = '\0';
        inst->enable = false;
        obj->writable_iface_failed = false;
    } else {
        return ANJAY_ERR_METHOD_NOT_ALLOWED;
    }
    return 0;
}

static int list_resources(anjay_t *anjay,
                          const anjay_dm_object_def_t *const *obj_ptr,
                          anjay_iid_t iid,
                          anjay_dm_resource_list_ctx_t *ctx) {
    (void) anjay;
    (void) obj_ptr;
    (void) iid;

    anjay_dm_emit_res(ctx, RID_INTERFACE_NAME, ANJAY_DM_RES_R,
                      ANJAY_DM_RES_PRESENT);
    anjay_dm_emit_res(ctx, RID_ENABLE, ANJAY_DM_RES_RW, ANJAY_DM_RES_PRESENT);
    anjay_dm_emit_res(ctx, RID_STATUS, ANJAY_DM_RES_R, ANJAY_DM_RES_PRESENT);
    anjay_dm_emit_res(ctx, RID_BSSID, ANJAY_DM_RES_R, ANJAY_DM_RES_PRESENT);
    anjay_dm_emit_res(ctx, RID_SSID, ANJAY_DM_RES_RW, ANJAY_DM_RES_PRESENT);
    anjay_dm_emit_res(ctx, RID_MODE, ANJAY_DM_RES_R, ANJAY_DM_RES_PRESENT);
    anjay_dm_emit_res(ctx, RID_CHANNEL, ANJAY_DM_RES_R, ANJAY_DM_RES_PRESENT);
    anjay_dm_emit_res(ctx, RID_STANDARD, ANJAY_DM_RES_R, ANJAY_DM_RES_PRESENT);
    anjay_dm_emit_res(ctx, RID_AUTHENTICATION_MODE, ANJAY_DM_RES_R,
                      ANJAY_DM_RES_PRESENT);
    anjay_dm_emit_res(ctx, RID_WPA_KEY_PHRASE, ANJAY_DM_RES_W,
                      ANJAY_DM_RES_PRESENT);
    return 0;
}

static int resource_read(anjay_t *anjay,
                         const anjay_dm_object_def_t *const *obj_ptr,
                         anjay_iid_t iid,
                         anjay_rid_t rid,
                         anjay_riid_t riid,
                         anjay_output_ctx_t *ctx) {
    (void) anjay;

    wlan_connectivity_object_t *obj = get_obj(obj_ptr);
    assert(obj);
    assert(iid < AVS_ARRAY_SIZE(obj->instances));
    wlan_connectivity_instance_t *inst = &obj->instances[iid];
    switch (rid) {
    case RID_INTERFACE_NAME: {
        assert(riid == ANJAY_ID_INVALID);
        return anjay_ret_string(ctx,
                                iid == ANJAY_WIFI_OBJ_WRITABLE_INSTANCE
                                        ? "writable wlan config"
                                        : "preconfigured fallback");
    }

    case RID_ENABLE: {
        assert(riid == ANJAY_ID_INVALID);
        return anjay_ret_bool(ctx, inst->enable);
    }

    case RID_STATUS: {
        assert(riid == ANJAY_ID_INVALID);
        int32_t status = 0;
        if (iid == ANJAY_WIFI_OBJ_WRITABLE_INSTANCE
                && obj->writable_iface_failed) {
            status = 2;
        } else {
            status = inst->enable;
        }

        return anjay_ret_i32(ctx, status);
    }

    case RID_BSSID: {
        assert(riid == ANJAY_ID_INVALID);
        uint8_t mac[6];
        char aux_tab[12 + 1]; // 12 for MAC address, +1 for null terminator
        if (esp_wifi_get_mac(ESP_IF_WIFI_STA, mac) != ESP_OK) {
            return ANJAY_ERR_INTERNAL;
        }
        snprintf(aux_tab, AVS_ARRAY_SIZE(aux_tab), "%X%X%X%X%X%X", mac[0],
                 mac[1], mac[2], mac[3], mac[4], mac[5]);
        return anjay_ret_string(ctx, aux_tab);
    }

    case RID_SSID: {
        assert(riid == ANJAY_ID_INVALID);
        return anjay_ret_string(ctx, (char *) inst->wifi_config.sta.ssid);
    }

    case RID_MODE: {
        assert(riid == ANJAY_ID_INVALID);
        return anjay_ret_i32(ctx, 1); // 1 - Client (Station)
    }

    case RID_CHANNEL: {
        assert(riid == ANJAY_ID_INVALID);
        uint8_t primary_channel;
        wifi_second_chan_t ignored;
        if (esp_wifi_get_channel(&primary_channel, &ignored) != ESP_OK) {
            return ANJAY_ERR_INTERNAL;
        }
        return anjay_ret_i32(ctx, primary_channel);
    }

    case RID_STANDARD: {
        assert(riid == ANJAY_ID_INVALID);
        uint8_t protocol = 0;

        /**
         * As mentioned in ESP-IDF documentation
         * (https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_wifi.html)
         * currently there is only support for 802.11b or 802.11bg or
         * 802.11bgn mode
         */
        if (esp_wifi_get_protocol(ESP_IF_WIFI_STA, &protocol) != ESP_OK) {
            return ANJAY_ERR_INTERNAL;
        }
        if (protocol == WIFI_PROTOCOL_11B) {
            protocol = 1;
        } else if (protocol == (WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G)) {
            protocol = 2;
        } else if (protocol
                   == (WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G
                       | WIFI_PROTOCOL_11N)) {
            protocol = 5;
        }

        assert(protocol != 0);
        return anjay_ret_i32(ctx, protocol);
    }

    case RID_AUTHENTICATION_MODE: {
        assert(riid == ANJAY_ID_INVALID);
        return anjay_ret_i32(ctx, 1);
    }

    default:
        return ANJAY_ERR_METHOD_NOT_ALLOWED;
    }
}

static int resource_write(anjay_t *anjay,
                          const anjay_dm_object_def_t *const *obj_ptr,
                          anjay_iid_t iid,
                          anjay_rid_t rid,
                          anjay_riid_t riid,
                          anjay_input_ctx_t *ctx) {
    (void) anjay;

    wlan_connectivity_object_t *obj = get_obj(obj_ptr);
    assert(obj);
    assert(iid < AVS_ARRAY_SIZE(obj->instances));
    wlan_connectivity_instance_t *inst = &obj->instances[iid];
    if (iid == ANJAY_WIFI_OBJ_WRITABLE_INSTANCE) {
        switch (rid) {
        case RID_ENABLE: {
            assert(riid == ANJAY_ID_INVALID);
            return anjay_get_bool(ctx, &inst->enable);
        }

        case RID_SSID: {
            assert(riid == ANJAY_ID_INVALID);
            return anjay_get_string(ctx, (char *) inst->wifi_config.sta.ssid,
                                    AVS_ARRAY_SIZE(inst->wifi_config.sta.ssid));
        }

        case RID_WPA_KEY_PHRASE: {
            assert(riid == ANJAY_ID_INVALID);
            return anjay_get_string(
                    ctx, (char *) inst->wifi_config.sta.password,
                    AVS_ARRAY_SIZE(inst->wifi_config.sta.password));
        }

        default:
            return ANJAY_ERR_METHOD_NOT_ALLOWED;
        }
    } else {
        return ANJAY_ERR_METHOD_NOT_ALLOWED;
    }
}

static int nvs_write_u8(const char *namespace, const char *key, uint8_t val) {
    nvs_handle_t nvs_h;
    int result = 0;

    avs_log(tutorial, INFO, "Opening Non-Volatile Storage (NVS) handle... ");
    result = nvs_open(namespace, NVS_READWRITE, &nvs_h);
    if (result != ESP_OK) {
        avs_log(tutorial, ERROR, "Error (%s) opening NVS handle!",
                esp_err_to_name(result));
        return ANJAY_ERR_INTERNAL;
    }

    if (nvs_set_u8(nvs_h, key, val) != ESP_OK || nvs_commit(nvs_h) != ESP_OK) {
        avs_log(tutorial, ERROR, "Error during saving new value in NVS");
        result = ANJAY_ERR_INTERNAL;
    }
    nvs_close(nvs_h);
    return result;
}

static int
nvs_write_str(const char *namespace, const char *key, const char *val) {
    nvs_handle_t nvs_h;
    int result = 0;

    avs_log(tutorial, INFO, "Opening Non-Volatile Storage (NVS) handle... ");
    result = nvs_open(namespace, NVS_READWRITE, &nvs_h);
    if (result != ESP_OK) {
        avs_log(tutorial, ERROR, "Error (%s) opening NVS handle!",
                esp_err_to_name(result));
        return ANJAY_ERR_INTERNAL;
    }

    if (nvs_set_str(nvs_h, key, val) != ESP_OK || nvs_commit(nvs_h) != ESP_OK) {
        avs_log(tutorial, ERROR, "Error during saving new value in NVS");
        result = ANJAY_ERR_INTERNAL;
    }
    nvs_close(nvs_h);
    return result;
}

static int transaction_begin(anjay_t *anjay,
                             const anjay_dm_object_def_t *const *obj_ptr) {
    (void) anjay;

    wlan_connectivity_object_t *obj = get_obj(obj_ptr);
    assert(obj);
    wlan_connectivity_instance_t *inst =
            &obj->instances[ANJAY_WIFI_OBJ_WRITABLE_INSTANCE];

    inst->wifi_config_backup = inst->wifi_config;
    inst->enable_backup = inst->enable;
    return 0;
}

static int transaction_rollback(anjay_t *anjay,
                                const anjay_dm_object_def_t *const *obj_ptr) {
    (void) anjay;

    wlan_connectivity_object_t *obj = get_obj(obj_ptr);
    assert(obj);
    wlan_connectivity_instance_t *inst =
            &obj->instances[ANJAY_WIFI_OBJ_WRITABLE_INSTANCE];

    inst->wifi_config = inst->wifi_config_backup;
    inst->enable = inst->enable_backup;
    return 0;
}

static int transaction_commit(anjay_t *anjay,
                              const anjay_dm_object_def_t *const *obj_ptr) {
    (void) anjay;

    wlan_connectivity_object_t *obj = get_obj(obj_ptr);
    assert(obj);
    wlan_connectivity_instance_t *inst =
            &obj->instances[ANJAY_WIFI_OBJ_WRITABLE_INSTANCE];

    if (inst->enable != inst->enable_backup) {
        schedule_change_config();
        nvs_write_u8(MAIN_NVS_WRITABLE_WIFI_CONFIG_NAMESPACE,
                     MAIN_NVS_ENABLE_KEY, (uint8_t) inst->enable);
        nvs_write_u8(MAIN_NVS_CONFIG_NAMESPACE, MAIN_NVS_ENABLE_KEY,
                     (uint8_t) (!inst->enable));
    }

    if (strcmp((char *) inst->wifi_config.sta.ssid,
               (char *) inst->wifi_config_backup.sta.ssid)) {
        if (inst->enable) {
            schedule_change_config();
        }
        nvs_write_str(MAIN_NVS_WRITABLE_WIFI_CONFIG_NAMESPACE,
                      MAIN_NVS_WIFI_SSID_KEY,
                      (char *) inst->wifi_config.sta.ssid);
    }

    if (strcmp((char *) inst->wifi_config.sta.password,
               (char *) inst->wifi_config_backup.sta.password)) {
        if (inst->enable) {
            schedule_change_config();
        }
        nvs_write_str(MAIN_NVS_WRITABLE_WIFI_CONFIG_NAMESPACE,
                      MAIN_NVS_WIFI_PASSWORD_KEY,
                      (char *) inst->wifi_config.sta.password);
    }
    return 0;
}

static const anjay_dm_object_def_t OBJ_DEF = {
    .oid = OID_WLAN_CONNECTIVITY,
    .handlers = {
        .list_instances = list_instances,
        .instance_reset = instance_reset,

        .list_resources = list_resources,
        .resource_read = resource_read,
        .resource_write = resource_write,

        .transaction_begin = transaction_begin,
        .transaction_validate = anjay_dm_transaction_NOOP,
        .transaction_commit = transaction_commit,
        .transaction_rollback = transaction_rollback
    }
};

const anjay_dm_object_def_t **wlan_object_create(void) {
    wlan_connectivity_object_t *obj = (wlan_connectivity_object_t *) avs_calloc(
            1, sizeof(wlan_connectivity_object_t));
    if (!obj) {
        return NULL;
    }
    obj->def = &OBJ_DEF;

    return &obj->def;
}

void wlan_object_release(const anjay_dm_object_def_t **def) {
    if (def) {
        wlan_connectivity_object_t *obj = get_obj(def);

        avs_free(obj);
    }
}

void wlan_object_set_instance_wifi_config(
        const anjay_t *anjay,
        const anjay_dm_object_def_t *const *obj_ptr,
        wifi_instance_t iid,
        wifi_config_t *conf) {
    wlan_connectivity_object_t *obj = get_obj(obj_ptr);
    assert(obj);
    assert(iid < AVS_ARRAY_SIZE(obj->instances));
    wlan_connectivity_instance_t *inst = &obj->instances[iid];

    if (strcmp((char *) inst->wifi_config.sta.ssid, (char *) conf->sta.ssid)) {
        anjay_notify_changed((anjay_t *) anjay, OID_WLAN_CONNECTIVITY, iid,
                             RID_SSID);
    }

    if (strcmp((char *) inst->wifi_config.sta.password,
               (char *) conf->sta.password)) {
        anjay_notify_changed((anjay_t *) anjay, OID_WLAN_CONNECTIVITY, iid,
                             RID_WPA_KEY_PHRASE);
    }

    inst->wifi_config = *conf;
}

wifi_config_t wlan_object_get_instance_wifi_config(
        const anjay_dm_object_def_t *const *obj_ptr, wifi_instance_t iid) {
    wlan_connectivity_object_t *obj = get_obj(obj_ptr);
    assert(obj);
    assert(iid < AVS_ARRAY_SIZE(obj->instances));
    wlan_connectivity_instance_t *inst = &obj->instances[iid];
    return inst->wifi_config;
}

void wlan_object_set_instance_enable(
        const anjay_t *anjay,
        const anjay_dm_object_def_t *const *obj_ptr,
        wifi_instance_t iid,
        bool en) {
    wlan_connectivity_object_t *obj = get_obj(obj_ptr);
    assert(obj);
    assert(iid < AVS_ARRAY_SIZE(obj->instances));
    wlan_connectivity_instance_t *inst = &obj->instances[iid];

    if (inst->enable != en) {
        inst->enable = en;
        anjay_notify_changed((anjay_t *) anjay, OID_WLAN_CONNECTIVITY, iid,
                             RID_ENABLE);
        anjay_notify_changed((anjay_t *) anjay, OID_WLAN_CONNECTIVITY, iid,
                             RID_STATUS);

        if (iid == ANJAY_WIFI_OBJ_WRITABLE_INSTANCE) {
            nvs_write_u8(MAIN_NVS_WRITABLE_WIFI_CONFIG_NAMESPACE,
                         MAIN_NVS_ENABLE_KEY, (uint8_t) en);
        } else {
            nvs_write_u8(MAIN_NVS_CONFIG_NAMESPACE, MAIN_NVS_ENABLE_KEY,
                         (uint8_t) en);
        }
    }
}

bool wlan_object_is_instance_enabled(
        const anjay_dm_object_def_t *const *obj_ptr, wifi_instance_t iid) {
    wlan_connectivity_object_t *obj = get_obj(obj_ptr);
    assert(obj);
    assert(iid < AVS_ARRAY_SIZE(obj->instances));
    wlan_connectivity_instance_t *inst = &obj->instances[iid];
    return inst->enable;
}

void wlan_object_set_writable_iface_failed(
        const anjay_t *anjay,
        const anjay_dm_object_def_t *const *obj_ptr,
        bool val) {
    wlan_connectivity_object_t *obj = get_obj(obj_ptr);
    assert(obj);

    if (obj->writable_iface_failed != val) {
        obj->writable_iface_failed = val;
        anjay_notify_changed((anjay_t *) anjay, OID_WLAN_CONNECTIVITY,
                             ANJAY_WIFI_OBJ_WRITABLE_INSTANCE, RID_STATUS);
    }
}
