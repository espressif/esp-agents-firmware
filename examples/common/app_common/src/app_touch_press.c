/**
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <esp_check.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <agent_setup.h>

#include "app_device.h"
#include "app_touch_press.h"

static const char *TAG = "app_touch_press";

#define TOUCH_LONG_PRESS_DURATION_US (10ULL * 1000 * 1000)

static esp_timer_handle_t s_touch_press_timer = NULL;
static bool s_factory_reset_triggered = false;

static void touch_press_timeout_cb(void *arg)
{
    ESP_LOGW(TAG, "Touch held for %llu ms, enqueue factory reset",
             (unsigned long long)(TOUCH_LONG_PRESS_DURATION_US / 1000ULL));
    s_factory_reset_triggered = true;
    app_device_event_enqueue(DEVICE_EVENT_FACTORY_RESET);
}

esp_err_t app_touch_press_init(void)
{
    if (s_touch_press_timer != NULL) {
        ESP_LOGW(TAG, "Touch press handler already initialized");
        return ESP_OK;
    }

    esp_timer_create_args_t timer_args = {
        .callback = touch_press_timeout_cb,
        .arg = NULL,
        .name = "touch_press",
    };
    ESP_RETURN_ON_ERROR(esp_timer_create(&timer_args, &s_touch_press_timer), TAG,
                        "Failed to create touch press timer");
    
    return ESP_OK;
}

bool app_touch_press_on_active(void)
{
    if (s_touch_press_timer) {
        s_factory_reset_triggered = false;
        esp_timer_stop(s_touch_press_timer);
        esp_timer_start_once(s_touch_press_timer, TOUCH_LONG_PRESS_DURATION_US);
    }
    return false;
}

bool app_touch_press_on_inactive(void)
{
    if (s_touch_press_timer) {
        esp_timer_stop(s_touch_press_timer);
    }

    if (!s_factory_reset_triggered) {
        app_device_event_enqueue(DEVICE_EVENT_INTERRUPT);
    } else {
        agent_setup_factory_reset();
    }

    s_factory_reset_triggered = false;
    return false;
}

void app_touch_press_deinit(void)
{
    if (s_touch_press_timer) {
        esp_timer_stop(s_touch_press_timer);
        esp_timer_delete(s_touch_press_timer);
        s_touch_press_timer = NULL;
    }
    s_factory_reset_triggered = false;
}
