/**
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <esp_log.h>
#include <esp_check.h>
#include <driver/touch_sens.h>
#include <driver/gpio.h>

#include "app_capacitive_touch.h"
#include "app_touch_press.h"
#include "board_defs.h"

#if CAPACITIVE_TOUCH_SUPPORTED
static const char *TAG = "app_touch";

static bool touch_on_active(touch_sensor_handle_t sens_handle, const touch_active_event_data_t *event, void *user_ctx)
{
    ESP_LOGI(TAG, "Capacitive touch detected");
    return app_touch_press_on_active();
}

static bool touch_on_inactive(touch_sensor_handle_t sens_handle, const touch_inactive_event_data_t *event, void *user_ctx)
{
    ESP_LOGI(TAG, "Capacitive touch inactive");
    return app_touch_press_on_inactive();
}

#define TOUCH_CHAN_INIT_SCAN_TIMES 3
esp_err_t app_capacitive_touch_init()
{
    ESP_LOGI(TAG, "Initializing touch sensor on GPIO %d", CAPACITIVE_TOUCH_CHANNEL_GPIO);

    touch_sensor_handle_t touch_handle;
    touch_channel_handle_t touch_chan_handle;

    touch_sensor_sample_config_t sample_cfg[TOUCH_SAMPLE_CFG_NUM] = {
        TOUCH_SENSOR_V2_DEFAULT_SAMPLE_CONFIG(500, TOUCH_VOLT_LIM_L_0V5, TOUCH_VOLT_LIM_H_2V7),
    };

    touch_sensor_config_t sens_cfg = TOUCH_SENSOR_DEFAULT_BASIC_CONFIG(1, sample_cfg);
    ESP_RETURN_ON_ERROR(touch_sensor_new_controller(&sens_cfg, &touch_handle), TAG, "Failed to create touch sensor controller");

    touch_channel_config_t chan_cfg = {
        .active_thresh = {2000},
        .charge_speed = TOUCH_CHARGE_SPEED_7,
        .init_charge_volt = TOUCH_INIT_CHARGE_VOLT_DEFAULT,
    };

    ESP_RETURN_ON_ERROR(touch_sensor_new_channel(touch_handle, CAPACITIVE_TOUCH_CHANNEL_GPIO,
                        &chan_cfg, &touch_chan_handle), TAG, "Failed to create touch channel");

    touch_sensor_filter_config_t filter_cfg = TOUCH_SENSOR_DEFAULT_FILTER_CONFIG();
    ESP_RETURN_ON_ERROR(touch_sensor_config_filter(touch_handle, &filter_cfg), TAG, "Failed to configure touch filter");

    // Step 5: Do initial scanning to initialize channel data
    ESP_LOGI(TAG, "Performing initial touch sensor scanning...");
    ESP_RETURN_ON_ERROR(touch_sensor_enable(touch_handle), TAG, "Failed to enable touch sensor");

    // Scan multiple times to stabilize the data
    for (int i = 0; i < TOUCH_CHAN_INIT_SCAN_TIMES; i++) {
        ESP_RETURN_ON_ERROR(touch_sensor_trigger_oneshot_scanning(touch_handle, 2000), TAG, "Failed to trigger scanning");
    }

    ESP_RETURN_ON_ERROR(touch_sensor_disable(touch_handle), TAG, "Failed to disable touch sensor");

    // Step 6: Read benchmark and reconfigure threshold
    uint32_t benchmark[1] = {0};

    ESP_RETURN_ON_ERROR(touch_channel_read_data(touch_chan_handle, TOUCH_CHAN_DATA_TYPE_BENCHMARK, benchmark), TAG, "Failed to read benchmark");

    // Calculate threshold as 1.5% of benchmark
    float thresh_ratio = 0.015f;

    chan_cfg.active_thresh[0] = (uint32_t)(benchmark[0] * thresh_ratio);
    ESP_LOGI(TAG, "Touch sensor benchmark: %lu, threshold: %lu", benchmark[0], chan_cfg.active_thresh[0]);

    // Update channel configuration with calculated threshold
    ESP_RETURN_ON_ERROR(touch_sensor_reconfig_channel(touch_chan_handle, &chan_cfg), TAG, "Failed to reconfigure channel");

    // Step 7: Register callbacks
    touch_event_callbacks_t cbs = {
        .on_active = touch_on_active,
        .on_inactive = touch_on_inactive,
    };
    ESP_RETURN_ON_ERROR(touch_sensor_register_callbacks(touch_handle, &cbs, NULL), TAG, "Failed to register touch callbacks");

    // Step 8: Enable and start continuous scanning
    ESP_RETURN_ON_ERROR(touch_sensor_enable(touch_handle), TAG, "Failed to enable touch sensor");
    ESP_RETURN_ON_ERROR(touch_sensor_start_continuous_scanning(touch_handle), TAG, "Failed to start continuous scanning");

    ESP_LOGI(TAG, "Touch sensor initialized successfully");
    return ESP_OK;
}

#else

esp_err_t app_capacitive_touch_init()
{
    return ESP_OK;
}

#endif
