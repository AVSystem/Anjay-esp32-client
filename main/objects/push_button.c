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

#include <assert.h>
#include <stdbool.h>

#include <anjay/anjay.h>
#include <avsystem/commons/avs_defs.h>
#include <avsystem/commons/avs_list.h>
#include <avsystem/commons/avs_memory.h>

#include "driver/gpio.h"

#include "objects.h"

/**
 * Digital Input State: R, Single, Mandatory
 * type: boolean, range: N/A, unit: N/A
 * The current state of a digital input.
 */
#define RID_DIGITAL_INPUT_STATE 5500

/**
 * Digital Input Counter: R, Single, Optional
 * type: integer, range: N/A, unit: N/A
 * The cumulative value of active state detected.
 */
#define RID_DIGITAL_INPUT_COUNTER 5501

#ifdef CONFIG_ANJAY_CLIENT_PUSH_BUTTON

#    define BUTTON_IID 0

typedef struct push_button_object_struct {
    const anjay_dm_object_def_t *def;

    bool digital_input_state;
    bool digital_input_state_last;
    int digital_input_counter;
    bool digital_input_counter_changed;
} push_button_object_t;

static void digital_input_state_changed(void *_obj) {
    push_button_object_t *obj = (push_button_object_t *) _obj;

    obj->digital_input_state =
            !gpio_get_level(CONFIG_ANJAY_CLIENT_PUSH_BUTTON_PIN);
    if (obj->digital_input_state) {
        obj->digital_input_counter++;
        obj->digital_input_counter_changed = true;
    }
}

static inline push_button_object_t *
get_obj(const anjay_dm_object_def_t *const *obj_ptr) {
    assert(obj_ptr);
    return AVS_CONTAINER_OF(obj_ptr, push_button_object_t, def);
}

static int list_resources(anjay_t *anjay,
                          const anjay_dm_object_def_t *const *obj_ptr,
                          anjay_iid_t iid,
                          anjay_dm_resource_list_ctx_t *ctx) {
    (void) anjay;
    (void) obj_ptr;
    (void) iid;

    anjay_dm_emit_res(ctx, RID_DIGITAL_INPUT_STATE, ANJAY_DM_RES_R,
                      ANJAY_DM_RES_PRESENT);
    anjay_dm_emit_res(ctx, RID_DIGITAL_INPUT_COUNTER, ANJAY_DM_RES_R,
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

    push_button_object_t *obj = get_obj(obj_ptr);
    assert(obj);

    switch (rid) {
    case RID_DIGITAL_INPUT_STATE:
        assert(riid == ANJAY_ID_INVALID);
        return anjay_ret_bool(ctx, obj->digital_input_state);

    case RID_DIGITAL_INPUT_COUNTER:
        assert(riid == ANJAY_ID_INVALID);
        return anjay_ret_i32(ctx, obj->digital_input_counter);

    default:
        return ANJAY_ERR_METHOD_NOT_ALLOWED;
    }
}

static const anjay_dm_object_def_t OBJ_DEF = {
    .oid = 3347,
    .handlers = {
        .list_instances = anjay_dm_list_instances_SINGLE,
        .list_resources = list_resources,
        .resource_read = resource_read,
    }
};

static const anjay_dm_object_def_t *OBJ_DEF_PTR = &OBJ_DEF;

void push_button_object_release(const anjay_dm_object_def_t **def) {
    if (def) {
        push_button_object_t *obj = get_obj(def);
        gpio_reset_pin(0);
        gpio_isr_handler_remove(0);
        gpio_uninstall_isr_service();

        avs_free(obj);
    }
}

const anjay_dm_object_def_t **push_button_object_create(void) {
    push_button_object_t *obj =
            (push_button_object_t *) avs_calloc(1,
                                                sizeof(push_button_object_t));
    if (!obj) {
        return NULL;
    }
    obj->def = OBJ_DEF_PTR;

    obj->digital_input_counter = 0;
    obj->digital_input_counter_changed = false;
    obj->digital_input_state = false;
    obj->digital_input_state_last = false;

    gpio_config_t config = {
        .pin_bit_mask = 1,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = true,
        .pull_down_en = false,
        .intr_type = GPIO_INTR_ANYEDGE
    };

    if (gpio_config(&config) || gpio_install_isr_service(0)
            || gpio_isr_handler_add(0, digital_input_state_changed, obj)) {
        push_button_object_release(&OBJ_DEF_PTR);
        gpio_reset_pin(0);
        return NULL;
    }

    return &obj->def;
}

void push_button_object_update(anjay_t *anjay,
                               const anjay_dm_object_def_t *const *def) {
    if (!anjay || !def) {
        return;
    }

    push_button_object_t *obj = get_obj(def);

    if (obj->digital_input_counter_changed) {
        obj->digital_input_counter_changed = false;
        (void) anjay_notify_changed(anjay, obj->def->oid, BUTTON_IID,
                                    RID_DIGITAL_INPUT_COUNTER);
    }

    if (obj->digital_input_state_last != obj->digital_input_state) {
        obj->digital_input_state_last = obj->digital_input_state;
        (void) anjay_notify_changed(anjay, obj->def->oid, BUTTON_IID,
                                    RID_DIGITAL_INPUT_STATE);
    }
}
#else  // CONFIG_ANJAY_CLIENT_PUSH_BUTTON
const anjay_dm_object_def_t **push_button_object_create(void) {
    return NULL;
}

void push_button_object_release(const anjay_dm_object_def_t **def) {
    (void) def;
}

void push_button_object_update(anjay_t *anjay,
                               const anjay_dm_object_def_t *const *def) {
    (void) anjay;
    (void) def;
}
#endif // CONFIG_ANJAY_CLIENT_PUSH_BUTTON
