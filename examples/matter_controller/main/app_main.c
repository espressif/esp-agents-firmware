/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <esp_log.h>
#include <nvs_flash.h>
#include <esp_check.h>

#include <agent_console.h>

#include <agent_setup.h>
#include <setup/rainmaker.h>
#include <setup/console.h>

#include <esp_board_manager.h>

#include <app_agent.h>
#include <app_device.h>
#include <app_display.h>
#include <app_controller.h>
#include <board_defs.h>

#include <app_audio.h>
#include "app_tools.h"

static const char *TAG = "main";

void app_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    /* Nothing specific to do here */
    ESP_LOGD(TAG, "Event received: %d, event ID: %d", event_base, event_id);

    /* Forward the event to the common agent event handler */
    app_agent_default_event_handler(arg, event_base, event_id, event_data);
}

esp_err_t app_text_message_callback(app_device_text_type_t text_type, const char *text, void *priv_data)
{
    ESP_LOGD(TAG, "Text message: %s, text type: %d", text, text_type);
    if (text_type != APP_DEVICE_TEXT_TYPE_SYSTEM) {
        char *role = text_type == APP_DEVICE_TEXT_TYPE_USER ? ">>" : "<<";
        printf("%s %s\n", role, text);
    }

    /* Sending to the display */
    app_display_set_text(text_type, text, priv_data);
    return ESP_OK;
}

esp_err_t app_state_changed_callback(app_device_system_state_t new_state, void *priv_data)
{
    ESP_LOGD(TAG, "System state: %d", new_state);

    /* Sending to the display */
    app_display_system_state_changed(new_state, priv_data);
    return ESP_OK;
}

void app_main(void)
{
    /* Initialize the event loop and NVS */
    ESP_RETURN_VOID_ON_ERROR(esp_event_loop_create_default(), TAG, "Failed to create event loop");
    ESP_RETURN_VOID_ON_ERROR(nvs_flash_init(), TAG, "Failed to initialize NVS");

    /* Initialize the console and register default commands */
    ESP_RETURN_VOID_ON_ERROR(agent_console_init(), TAG, "Failed to initialize console");

    /* Initialize the peripherals: i2c, i2s, spi, etc. */
    ESP_RETURN_VOID_ON_ERROR(esp_board_manager_init(), TAG, "Failed to initialize board manager");

    /* Initialize the display */
    ESP_RETURN_VOID_ON_ERROR(app_display_init(), TAG, "Failed to initialize display");

    /* Initialize the audio pipeline: microphone and speaker */
    ESP_RETURN_VOID_ON_ERROR(app_audio_init(), TAG, "Failed to initialize audio pipeline");

    /* State machine. The default behaviour can be overridden by changing the callbacks. */
    app_device_config_t device_config = {
        .set_text_cb = app_text_message_callback,
        .system_state_changed_cb = app_state_changed_callback,
        .priv_data = NULL,
    };
    ESP_RETURN_VOID_ON_ERROR(app_device_init(&device_config), TAG, "Failed to initialize device");

    /* Initialize the agent: audio configuration, tools, event handler, etc. */
    app_agent_config_t agent_config = {
        .event_handler = app_event_handler,
    };
    ESP_RETURN_VOID_ON_ERROR(app_agent_init(&agent_config), TAG, "Failed to initialize agent");

    ESP_RETURN_VOID_ON_ERROR(app_tools_register(), TAG, "Failed to register tools");

    /* Start the agent: start network, connect to the cloud, etc. */
    ESP_RETURN_VOID_ON_ERROR(app_agent_start(), TAG, "Failed to start agent");

    ESP_RETURN_VOID_ON_ERROR(matter_controller_start_task(), TAG, "Failed to start Matter controller task");
}
