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

/*
 * Created basing on following references:
 *  - MPU6886 Datasheet:
 *    https://raw.githubusercontent.com/m5stack/M5-Schematic/master/datasheet/MPU-6886-000193%2Bv1.1_GHIC.PDF.pdf
 *  - I2C driver for ESP-IDF:
 *    https://gist.github.com/code0100fun/9e5335e9a36a3db9bd45453d77b336e4
 */
#include "mpu6886.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c_wrapper.h"
#include "objects.h"
#include "sdkconfig.h"
#include "stdbool.h"
#include "stdint.h"

#if CONFIG_ANJAY_CLIENT_BOARD_M5STICKC_PLUS

#    define I2C_MPU6886_ADDRESS (104)
#    define I2C_SDA_PIN (21)
#    define I2C_SCL_PIN (22)
#    define I2C_CLOCK_SPEED_HZ (400000)
#    define I2C_TIMEOUT_MS (10)
#    define I2C_TIMEOUT_TICKS (I2C_TIMEOUT_MS / portTICK_PERIOD_MS)

/*
 * MPU6886 registers addresses.
 *  - see datasheet, Table 15. MPU-6886 Register Map
 */
#    define MPU6886_REG_ADDR_CONFIG (0x1A)
#    define MPU6886_REG_ADDR_GYRO_CONFIG (0x1B)
#    define MPU6886_REG_ADDR_ACCEL_CONFIG (0x1C)
#    define MPU6886_REG_ADDR_ACCEL_CONFIG2 (0x1D)
#    define MPU6886_REG_ADDR_INTERRUPT_PIN (0x37)
#    define MPU6886_REG_ADDR_INTERRUPT_ENABLE (0x38)
#    define MPU6886_REG_ADDR_ACCEL_XOUT_H (0x3B)
#    define MPU6886_REG_ADDR_ACCEL_XOUT_L (0x3C)
#    define MPU6886_REG_ADDR_ACCEL_YOUT_H (0x3D)
#    define MPU6886_REG_ADDR_ACCEL_YOUT_L (0x3E)
#    define MPU6886_REG_ADDR_ACCEL_ZOUT_H (0x3F)
#    define MPU6886_REG_ADDR_ACCEL_ZOUT_L (0x40)
#    define MPU6886_REG_ADDR_TEMP_OUT_H (0x41)
#    define MPU6886_REG_ADDR_TEMP_OUT_L (0x42)
#    define MPU6886_REG_ADDR_GYRO_XOUT_H (0x43)
#    define MPU6886_REG_ADDR_GYRO_XOUT_L (0x44)
#    define MPU6886_REG_ADDR_GYRO_YOUT_H (0x45)
#    define MPU6886_REG_ADDR_GYRO_YOUT_L (0x46)
#    define MPU6886_REG_ADDR_GYRO_ZOUT_H (0x47)
#    define MPU6886_REG_ADDR_GYRO_ZOUT_L (0x48)
#    define MPU6886_REG_ADDR_PWR_MGMT_1 (0x6B)
#    define MPU6886_REG_ADDR_PWR_MGMT_2 (0x6C)
#    define MPU6886_REG_ADDR_WHO_AM_I (0x75)

/*
 * MPU6886 registers config values.
 *  - see datasheet, Chapter 8. Register Descriptions
 */
#    define MPU6886_REG_CONFIG_DEFAULT (0x00)
#    define MPU6886_REG_GYRO_CONFIG_FS_250DPS (0x00)
#    define MPU6886_REG_GYRO_CONFIG_FS_500DPS (0x08)
#    define MPU6886_REG_GYRO_CONFIG_FS_1000DPS (0x10)
#    define MPU6886_REG_GYRO_CONFIG_FS_2000DPS (0x18)
#    define MPU6886_REG_ACCEL_CONFIG_FS_2G (0x00)
#    define MPU6886_REG_ACCEL_CONFIG_FS_4G (0x08)
#    define MPU6886_REG_ACCEL_CONFIG_FS_8G (0x10)
#    define MPU6886_REG_ACCEL_CONFIG_FS_16G (0x18)
#    define MPU6886_REG_PWR_MGMT_2_EN_ALL (0x00)
#    define MPU6886_REG_WHO_AM_I_VAL (0x19)
#    define MPU6886_REG_PWR_MGMT_1_AUTO_SELECT_CLOCK (0x01)

/*
 * MPU6886 LSB output to real unit scaling factors.
 *  - see datasheet, Chapter 3. Elctrical Characteristics
 *                    - Table 1. Gysrocsope Specifications
 *                    - Table 2. Accelerometer Specifications
 *                    - Table 4. A.C. Electrical Characteristics
 */
#    define TEMPERATURE_LSB_TO_C_FACTOR (326.8)
#    define GYROSCOPE_LSB_TO_DPS_FACTOR_500DPS (62.5)
#    define ACCELEROMETER_LSB_TO_G_FACTOR_2G (16384.0)
#    define TEMPERATURE_ZERO_LSB_OFFSET (25.0)

/*
 * Macros for data structures access control
 * Mutexes guarantee datasets consistence
 */
#    define MUTEX_LOCK(struct) ((struct) .mutex = true)
#    define MUTEX_UNLOCK(struct) ((struct) .mutex = false)
#    define MUTEX_IS_LOCKED(struct) ((struct) .mutex)

typedef struct mems_sensor_data_struct {
    double x_value;
    double y_value;
    double z_value;
    bool mutex;
} mems_sensor_data_t;

typedef struct temperature_sensor_data_struct {
    double value;
    bool mutex;
} temperature_sensor_data_t;

static mems_sensor_data_t accelerometer_data, gyroscope_data;
static temperature_sensor_data_t temperature_sensor_data;

static i2c_device_t mpu6886_device = {
    .config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA_PIN,
        .scl_io_num = I2C_SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_CLOCK_SPEED_HZ
    },
    .port = I2C_MASTER_PORT,
    .address = I2C_MPU6886_ADDRESS
};

int accelerometer_read_data(void) {
    uint8_t acc[6];
    if ((i2c_master_read_slave_reg(&mpu6886_device,
                                   MPU6886_REG_ADDR_ACCEL_XOUT_H, acc, 6)
         == ESP_OK)
            || !MUTEX_IS_LOCKED(accelerometer_data)) {
        MUTEX_LOCK(accelerometer_data);
        accelerometer_data.x_value =
                ((double) (int16_t) ((acc[0] << 8) | acc[1]))
                / ACCELEROMETER_LSB_TO_G_FACTOR_2G * GRAVITY_CONSTANT;
        accelerometer_data.y_value =
                ((double) (int16_t) ((acc[2] << 8) | acc[3]))
                / ACCELEROMETER_LSB_TO_G_FACTOR_2G * GRAVITY_CONSTANT;
        accelerometer_data.z_value =
                ((double) (int16_t) ((acc[4] << 8) | acc[5]))
                / ACCELEROMETER_LSB_TO_G_FACTOR_2G * GRAVITY_CONSTANT;
        MUTEX_UNLOCK(accelerometer_data);
        return 0;
    } else {
        return -1;
    }
}

int accelerometer_get_data(three_axis_sensor_data_t *sensor_data) {
    if (!MUTEX_IS_LOCKED(accelerometer_data)) {
        MUTEX_LOCK(accelerometer_data);
        sensor_data->x_value = accelerometer_data.x_value;
        sensor_data->y_value = accelerometer_data.y_value;
        sensor_data->z_value = accelerometer_data.z_value;
        MUTEX_UNLOCK(accelerometer_data);
        return 0;
    } else {
        return -1;
    }
}

int temperature_read_data(void) {
    uint8_t temp[2];
    if ((i2c_master_read_slave_reg(&mpu6886_device, MPU6886_REG_ADDR_TEMP_OUT_H,
                                   temp, 2)
         == ESP_OK)
            || !MUTEX_IS_LOCKED(temperature_sensor_data)) {
        MUTEX_LOCK(temperature_sensor_data);
        temperature_sensor_data.value =
                ((double) ((int16_t) ((temp[0] << 8) | temp[1]))
                 / TEMPERATURE_LSB_TO_C_FACTOR)
                + TEMPERATURE_ZERO_LSB_OFFSET;
        MUTEX_UNLOCK(temperature_sensor_data);
        return 0;
    } else {
        return -1;
    }
}

int temperature_get_data(double *sensor_data) {
    if (!MUTEX_IS_LOCKED(temperature_sensor_data)) {
        MUTEX_LOCK(temperature_sensor_data);
        *sensor_data = temperature_sensor_data.value;
        MUTEX_UNLOCK(temperature_sensor_data);
        return 0;
    } else {
        return -1;
    }
}

int gyroscope_read_data(void) {
    uint8_t dps[6];
    if ((i2c_master_read_slave_reg(&mpu6886_device,
                                   MPU6886_REG_ADDR_GYRO_XOUT_H, dps, 6)
         == ESP_OK)
            || !MUTEX_IS_LOCKED(gyroscope_data)) {
        MUTEX_LOCK(gyroscope_data);
        gyroscope_data.x_value = ((double) (int16_t) ((dps[0] << 8) | dps[1]))
                                 / GYROSCOPE_LSB_TO_DPS_FACTOR_500DPS;
        gyroscope_data.y_value = ((double) (int16_t) ((dps[2] << 8) | dps[3]))
                                 / GYROSCOPE_LSB_TO_DPS_FACTOR_500DPS;
        gyroscope_data.z_value = ((double) (int16_t) ((dps[4] << 8) | dps[5]))
                                 / GYROSCOPE_LSB_TO_DPS_FACTOR_500DPS;
        MUTEX_UNLOCK(gyroscope_data);
        return 0;
    } else {
        return -1;
    }
}

int gyroscope_get_data(three_axis_sensor_data_t *sensor_data) {
    if (!MUTEX_IS_LOCKED(gyroscope_data)) {
        MUTEX_LOCK(gyroscope_data);
        sensor_data->x_value = gyroscope_data.x_value;
        sensor_data->y_value = gyroscope_data.y_value;
        sensor_data->z_value = gyroscope_data.z_value;
        MUTEX_UNLOCK(gyroscope_data);
        return 0;
    } else {
        return -1;
    }
}

int mpu6886_device_init(void) {
    i2c_device_init(&mpu6886_device);

    uint8_t data;
    if (i2c_master_read_slave_reg(&mpu6886_device, MPU6886_REG_ADDR_WHO_AM_I,
                                  &data, 1)
            || (data != MPU6886_REG_WHO_AM_I_VAL)) {
        return -1;
    }

    if (i2c_master_write_slave_reg(&mpu6886_device, MPU6886_REG_ADDR_CONFIG,
                                   MPU6886_REG_CONFIG_DEFAULT)) {
        return -1;
    }
    vTaskDelay(I2C_TIMEOUT_TICKS);

    if (i2c_master_write_slave_reg(&mpu6886_device,
                                   MPU6886_REG_ADDR_ACCEL_CONFIG,
                                   MPU6886_REG_ACCEL_CONFIG_FS_2G)) {
        return -1;
    }
    vTaskDelay(I2C_TIMEOUT_TICKS);

    if (i2c_master_write_slave_reg(&mpu6886_device,
                                   MPU6886_REG_ADDR_GYRO_CONFIG,
                                   MPU6886_REG_GYRO_CONFIG_FS_500DPS)) {
        return -1;
    }
    vTaskDelay(I2C_TIMEOUT_TICKS);

    if (i2c_master_write_slave_reg(&mpu6886_device, MPU6886_REG_ADDR_PWR_MGMT_2,
                                   MPU6886_REG_PWR_MGMT_2_EN_ALL)) {
        return -1;
    }
    vTaskDelay(I2C_TIMEOUT_TICKS);

    if (i2c_master_write_slave_reg(&mpu6886_device, MPU6886_REG_ADDR_PWR_MGMT_1,
                                   MPU6886_REG_PWR_MGMT_1_AUTO_SELECT_CLOCK)) {
        return -1;
    }

    MUTEX_UNLOCK(accelerometer_data);
    MUTEX_UNLOCK(gyroscope_data);
    MUTEX_UNLOCK(temperature_sensor_data);
    return 0;
}

void mpu6886_driver_release(void) {
    i2c_driver_delete(I2C_MASTER_PORT);
}

#endif /* CONFIG_ANJAY_CLIENT_BOARD_M5STICKC_PLUS */
