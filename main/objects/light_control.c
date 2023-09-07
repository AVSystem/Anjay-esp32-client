/*
 * Copyright 2021-2023 AVSystem <avsystem@avsystem.com>
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
#include <stdbool.h>

#include <anjay/anjay.h>
#include <avsystem/commons/avs_defs.h>
#include <avsystem/commons/avs_list.h>
#include <avsystem/commons/avs_memory.h>

#include <driver/ledc.h>

/**
 * On/Off: RW, Single, Mandatory
 * type: boolean, range: N/A, unit: N/A
 * On/off control. Boolean value where True is On and False is Off.
 */
#define RID_ON_OFF 5850

/**
 * Dimmer: RW, Single, Optional
 * type: integer, range: 0..100, unit: /100
 * This resource represents a dimmer setting, which has an Integer value
 * between 0 and 100 as a percentage.
 */
#define RID_DIMMER 5851

#if CONFIG_ANJAY_CLIENT_LIGHT_CONTROL
#    define DUTY_RESOLUTION LEDC_TIMER_10_BIT
#    define MAX_DUTY 1023 /* 2 ** DUTY_RESOLUTION - 1 */

static const int LED_GPIOS[] = {
#    if CONFIG_ANJAY_CLIENT_LIGHT_CONTROL_RED
    CONFIG_ANJAY_CLIENT_LIGHT_CONTROL_RED_PIN,
#    endif // CONFIG_ANJAY_CLIENT_LIGHT_CONTROL_RED

#    if CONFIG_ANJAY_CLIENT_LIGHT_CONTROL_GREEN
    CONFIG_ANJAY_CLIENT_LIGHT_CONTROL_GREEN_PIN,
#    endif // CONFIG_ANJAY_CLIENT_LIGHT_CONTROL_GREEN

#    if CONFIG_ANJAY_CLIENT_LIGHT_CONTROL_BLUE
    CONFIG_ANJAY_CLIENT_LIGHT_CONTROL_BLUE_PIN,
#    endif // CONFIG_ANJAY_CLIENT_LIGHT_CONTROL_BLUE
};

#    define LED_NUM AVS_ARRAY_SIZE(LED_GPIOS)

static const ledc_channel_t LED_CHANNELS[] = {
#    if CONFIG_ANJAY_CLIENT_LIGHT_CONTROL_RED
    LEDC_CHANNEL_0,
#    endif // CONFIG_ANJAY_CLIENT_LIGHT_CONTROL_RED

#    if CONFIG_ANJAY_CLIENT_LIGHT_CONTROL_GREEN
    LEDC_CHANNEL_1,
#    endif // CONFIG_ANJAY_CLIENT_LIGHT_CONTROL_GREEN

#    if CONFIG_ANJAY_CLIENT_LIGHT_CONTROL_BLUE
    LEDC_CHANNEL_2,
#    endif // CONFIG_ANJAY_CLIENT_LIGHT_CONTROL_BLUE
};

typedef struct light_control_instance_struct {
    anjay_iid_t iid;

    int gpio;
    ledc_channel_t channel;
    bool on;
    bool on_backup;
    int power;
    int power_backup;
} light_control_instance_t;

typedef struct light_control_object_struct {
    const anjay_dm_object_def_t *def;
    light_control_instance_t instances[LED_NUM];
} light_control_object_t;

static inline light_control_object_t *
get_obj(const anjay_dm_object_def_t *const *obj_ptr) {
    assert(obj_ptr);
    return AVS_CONTAINER_OF(obj_ptr, light_control_object_t, def);
}

static int rgb_led_set(light_control_instance_t *inst) {
    int real_power = !inst->on ? 0
                               : inst->power == 100
                                         ? MAX_DUTY
                                         : inst->power * MAX_DUTY / 100;
#    ifdef CONFIG_ANJAY_CLIENT_LIGHT_CONTROL_ACTIVE_LOW
    real_power = MAX_DUTY - real_power;
#    endif // CONFIG_ANJAY_CLIENT_LIGHT_CONTROL_ACTIVE_LOW

    if (ledc_set_duty(LEDC_LOW_SPEED_MODE, inst->channel, real_power)
            || ledc_update_duty(LEDC_LOW_SPEED_MODE, inst->channel)) {
        return ANJAY_ERR_INTERNAL;
    }

    return 0;
}

static int list_instances(anjay_t *anjay,
                          const anjay_dm_object_def_t *const *obj_ptr,
                          anjay_dm_list_ctx_t *ctx) {
    (void) anjay;

    for (anjay_iid_t iid = 0; iid < LED_NUM; iid++) {
        anjay_dm_emit(ctx, iid);
    }

    return 0;
}

static int init_instance(light_control_instance_t *inst,
                         anjay_iid_t iid,
                         int gpio,
                         ledc_channel_t channel) {
    assert(iid != ANJAY_ID_INVALID);

    inst->iid = iid;
    inst->gpio = gpio;
    inst->channel = channel;
    inst->power = 0;
    inst->on = false;

    return 0;
}

static int instance_reset(anjay_t *anjay,
                          const anjay_dm_object_def_t *const *obj_ptr,
                          anjay_iid_t iid) {
    (void) anjay;

    light_control_object_t *obj = get_obj(obj_ptr);
    assert(obj);

    obj->instances[iid].power = 0;
    obj->instances[iid].on = false;

    return rgb_led_set(&obj->instances[iid]);
}

static int list_resources(anjay_t *anjay,
                          const anjay_dm_object_def_t *const *obj_ptr,
                          anjay_iid_t iid,
                          anjay_dm_resource_list_ctx_t *ctx) {
    (void) anjay;
    (void) obj_ptr;
    (void) iid;

    anjay_dm_emit_res(ctx, RID_ON_OFF, ANJAY_DM_RES_RW, ANJAY_DM_RES_PRESENT);
    anjay_dm_emit_res(ctx, RID_DIMMER, ANJAY_DM_RES_RW, ANJAY_DM_RES_PRESENT);
    return 0;
}

static int resource_read(anjay_t *anjay,
                         const anjay_dm_object_def_t *const *obj_ptr,
                         anjay_iid_t iid,
                         anjay_rid_t rid,
                         anjay_riid_t riid,
                         anjay_output_ctx_t *ctx) {
    (void) anjay;

    light_control_object_t *obj = get_obj(obj_ptr);
    assert(obj);
    assert(iid < LED_NUM);

    switch (rid) {
    case RID_ON_OFF:
        assert(riid == ANJAY_ID_INVALID);
        return anjay_ret_bool(ctx, obj->instances[iid].on);

    case RID_DIMMER:
        assert(riid == ANJAY_ID_INVALID);
        return anjay_ret_i32(ctx, obj->instances[iid].power);

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

    light_control_object_t *obj = get_obj(obj_ptr);
    assert(obj);

    switch (rid) {
    case RID_ON_OFF: {
        assert(riid == ANJAY_ID_INVALID);
        return anjay_get_bool(ctx, &obj->instances[iid].on);
    }

    case RID_DIMMER: {
        assert(riid == ANJAY_ID_INVALID);
        return anjay_get_i32(ctx, &obj->instances[iid].power);
    }

    default:
        return ANJAY_ERR_METHOD_NOT_ALLOWED;
    }
}

static int transaction_begin(anjay_t *anjay,
                             const anjay_dm_object_def_t *const *obj_ptr) {
    (void) anjay;

    light_control_object_t *obj = get_obj(obj_ptr);

    for (anjay_iid_t iid = 0; iid < LED_NUM; iid++) {
        obj->instances[iid].on_backup = obj->instances[iid].on;
        obj->instances[iid].power_backup = obj->instances[iid].power;
    }

    return 0;
}

static int transaction_validate(anjay_t *anjay,
                                const anjay_dm_object_def_t *const *obj_ptr) {
    (void) anjay;

    light_control_object_t *obj = get_obj(obj_ptr);

    for (anjay_iid_t iid = 0; iid < LED_NUM; iid++) {
        if (obj->instances[iid].power < 0 || obj->instances[iid].power > 100) {
            return ANJAY_ERR_BAD_REQUEST;
        }
    }

    return 0;
}

static int transaction_commit(anjay_t *anjay,
                              const anjay_dm_object_def_t *const *obj_ptr) {
    (void) anjay;

    light_control_object_t *obj = get_obj(obj_ptr);

    for (anjay_iid_t iid = 0; iid < LED_NUM; iid++) {
        if (rgb_led_set(&obj->instances[iid])) {
            return -1;
        }
    }

    return 0;
}

static int transaction_rollback(anjay_t *anjay,
                                const anjay_dm_object_def_t *const *obj_ptr) {
    (void) anjay;

    light_control_object_t *obj = get_obj(obj_ptr);

    for (anjay_iid_t iid = 0; iid < LED_NUM; iid++) {
        obj->instances[iid].on = obj->instances[iid].on_backup;
        obj->instances[iid].power = obj->instances[iid].power_backup;
    }

    return 0;
}

static const anjay_dm_object_def_t OBJ_DEF = {
    .oid = 3311,
    .handlers = {
        .list_instances = list_instances,
        .instance_reset = instance_reset,

        .list_resources = list_resources,
        .resource_read = resource_read,
        .resource_write = resource_write,

        .transaction_begin = transaction_begin,
        .transaction_validate = transaction_validate,
        .transaction_commit = transaction_commit,
        .transaction_rollback = transaction_rollback
    }
};

void light_control_object_release(const anjay_dm_object_def_t **def) {
    if (def) {
        light_control_object_t *obj = get_obj(def);

        for (anjay_iid_t iid = 0; iid < LED_NUM; iid++) {
            obj->instances->on = false;
            rgb_led_set(obj->instances);
        }

        avs_free(obj);
    }
}

const anjay_dm_object_def_t **light_control_object_create(void) {
    light_control_object_t *obj = (light_control_object_t *) avs_calloc(
            1, sizeof(light_control_object_t));
    if (!obj) {
        return NULL;
    }
    obj->def = &OBJ_DEF;

    ledc_timer_config_t timer_config = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = DUTY_RESOLUTION,
        .timer_num = 0,
        .freq_hz = 2000,
        .clk_cfg = LEDC_AUTO_CLK
    };
    if (ledc_timer_config(&timer_config)) {
        return NULL;
    }

    for (anjay_iid_t iid = 0; iid < LED_NUM; iid++) {
        init_instance(&obj->instances[iid], iid, LED_GPIOS[iid],
                      LED_CHANNELS[iid]);

        ledc_channel_config_t channel_config = {
            .gpio_num = obj->instances[iid].gpio,
            .channel = obj->instances[iid].channel,
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .hpoint = 0
        };

        if (ledc_channel_config(&channel_config)
                || rgb_led_set(&obj->instances[iid])) {
            light_control_object_release(&obj->def);
            return NULL;
        }
    }

    return &obj->def;
}
#else  // CONFIG_ANJAY_CLIENT_LIGHT_CONTROL
const anjay_dm_object_def_t **light_control_object_create(void) {
    return NULL;
}

void light_control_object_release(const anjay_dm_object_def_t **def) {
    (void) def;
}

void light_control_object_update(anjay_t *anjay,
                                 const anjay_dm_object_def_t *const *def) {
    (void) anjay;
    (void) def;
}
#endif // CONFIG_ANJAY_CLIENT_LIGHT_CONTROL
