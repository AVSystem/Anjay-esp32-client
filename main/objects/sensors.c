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

#include <assert.h>
#include <stdbool.h>

#include <anjay/anjay.h>
#include <anjay/ipso_objects.h>

#include <avsystem/commons/avs_defs.h>
#include <avsystem/commons/avs_list.h>
#include <avsystem/commons/avs_log.h>
#include <avsystem/commons/avs_memory.h>

#include "mpu6886.h"
#include "objects/objects.h"
#include "sdkconfig.h"

typedef struct {
    const char *name;
    const char *unit;
    anjay_oid_t oid;
    double data;
    int (*read_data)(void);
    int (*get_data)(double *sensor_data);
} basic_sensor_context_t;

typedef struct {
    const char *name;
    const char *unit;
    anjay_oid_t oid;
    double min_value;
    double max_value;
    three_axis_sensor_data_t data;
    int (*read_data)(void);
    int (*get_data)(three_axis_sensor_data_t *sensor_data);
} three_axis_sensor_context_t;

static three_axis_sensor_context_t THREE_AXIS_SENSORS_DEF[] = {
#ifdef CONFIG_ANJAY_CLIENT_ACCELEROMETER_AVAILABLE
    {
        .name = "Accelerometer",
        .unit = "m/s2",
        .oid = 3313,
        .min_value = (-1.0) * ACCELEROMETER_RANGE * GRAVITY_CONSTANT,
        .max_value = ACCELEROMETER_RANGE * GRAVITY_CONSTANT,
        .read_data = accelerometer_read_data,
        .get_data = accelerometer_get_data,
    },
#endif // CONFIG_ANJAY_CLIENT_ACCELEROMETER_AVAILABLE
#ifdef CONFIG_ANJAY_CLIENT_GYROSCOPE_AVAILABLE
    {
        .name = "Gyroscope",
        .unit = "deg/s",
        .oid = 3334,
        .min_value = (-1.0) * GYROSCOPE_RANGE,
        .max_value = GYROSCOPE_RANGE,
        .read_data = gyroscope_read_data,
        .get_data = gyroscope_get_data,
    },
#endif // CONFIG_ANJAY_CLIENT_GYROSCOPE_AVAILABLE
};

static basic_sensor_context_t BASIC_SENSORS_DEF[] = {
#ifdef CONFIG_ANJAY_CLIENT_TEMPERATURE_SENSOR_AVAILABLE
    {
        .name = "Temperature sensor",
        .unit = "Cel",
        .oid = 3303,
        .read_data = temperature_read_data,
        .get_data = temperature_get_data,
    },
#endif // CONFIG_ANJAY_CLIENT_TEMPERATURE_SENSOR_AVAILABLE
};

int basic_sensor_get_value(anjay_iid_t iid, void *_ctx, double *value) {
    basic_sensor_context_t *ctx = (basic_sensor_context_t *) _ctx;

    assert(ctx->read_data);
    assert(ctx->get_data);
    assert(value);

    if (!ctx->read_data() && !ctx->get_data(&ctx->data)) {
        *value = ctx->data;
        return 0;
    } else {
        return -1;
    }
}

int three_axis_sensor_get_values(anjay_iid_t iid,
                                 void *_ctx,
                                 double *x_value,
                                 double *y_value,
                                 double *z_value) {
    three_axis_sensor_context_t *ctx = (three_axis_sensor_context_t *) _ctx;

    assert(ctx->read_data);
    assert(ctx->get_data);
    assert(x_value);
    assert(y_value);
    assert(z_value);

    if (!ctx->read_data() && !ctx->get_data(&ctx->data)) {
        *x_value = ctx->data.x_value;
        *y_value = ctx->data.y_value;
        *z_value = ctx->data.z_value;
        return 0;
    } else {
        return -1;
    }
}

void sensors_install(anjay_t *anjay) {
#if CONFIG_ANJAY_CLIENT_BOARD_M5STICKC_PLUS
    if (mpu6886_device_init()) {
        avs_log(ipso_object,
                WARNING,
                "Driver for MPU6886 could not be initialized!");
        return;
    }
#endif

    for (int i = 0; i < (int) AVS_ARRAY_SIZE(BASIC_SENSORS_DEF); i++) {
        basic_sensor_context_t *ctx = &BASIC_SENSORS_DEF[i];

        if (anjay_ipso_basic_sensor_install(anjay, ctx->oid, 1)) {
            avs_log(ipso_object,
                    WARNING,
                    "Object: %s could not be installed",
                    ctx->name);
            continue;
        }

        if (anjay_ipso_basic_sensor_instance_add(
                    anjay,
                    ctx->oid,
                    0,
                    (anjay_ipso_basic_sensor_impl_t) {
                        .unit = ctx->unit,
                        .user_context = ctx,
                        .min_range_value = NAN,
                        .max_range_value = NAN,
                        .get_value = basic_sensor_get_value
                    })) {
            avs_log(ipso_object,
                    WARNING,
                    "Instance of %s object could not be added",
                    ctx->name);
        }
    }
    for (int i = 0; i < (int) AVS_ARRAY_SIZE(THREE_AXIS_SENSORS_DEF); i++) {
        three_axis_sensor_context_t *ctx = &THREE_AXIS_SENSORS_DEF[i];

        if (anjay_ipso_3d_sensor_install(anjay, ctx->oid, 1)) {
            avs_log(ipso_object,
                    WARNING,
                    "Object: %s could not be installed",
                    ctx->name);
            continue;
        }

        if (anjay_ipso_3d_sensor_instance_add(
                    anjay,
                    ctx->oid,
                    0,
                    (anjay_ipso_3d_sensor_impl_t) {
                        .unit = ctx->unit,
                        .use_y_value = true,
                        .use_z_value = true,
                        .user_context = ctx,
                        .min_range_value = ctx->min_value,
                        .max_range_value = ctx->max_value,
                        .get_values = three_axis_sensor_get_values
                    })) {
            avs_log(ipso_object,
                    WARNING,
                    "Instance of %s object could not be added",
                    ctx->name);
        }
    }
}

void sensors_update(anjay_t *anjay) {
    for (int i = 0; i < (int) AVS_ARRAY_SIZE(BASIC_SENSORS_DEF); i++) {
        anjay_ipso_basic_sensor_update(anjay, BASIC_SENSORS_DEF[i].oid, 0);
    }
    for (int i = 0; i < (int) AVS_ARRAY_SIZE(THREE_AXIS_SENSORS_DEF); i++) {
        anjay_ipso_3d_sensor_update(anjay, THREE_AXIS_SENSORS_DEF[i].oid, 0);
    }
}

void sensors_release(void) {
#if CONFIG_ANJAY_CLIENT_BOARD_M5STICKC_PLUS
    mpu6886_driver_release();
#endif
}
