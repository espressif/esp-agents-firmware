/**
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <esp_timer.h>
#include <string.h>

#include <esp_check.h>
#include <esp_log.h>
#include <time.h>

#include "app_common_tools.h"
#include "app_audio.h"
#include "app_device.h"

static const char *TAG = "app_common_tools";

static void reminder_timer_callback(void *arg)
{
    char *task = (char *)arg;
    ESP_LOGI(TAG, "Reminder expired: %s", task);

    char *task_copy = strdup(task);
    if (!task_copy) {
        ESP_LOGE(TAG, "Failed to allocate memory for reminder task");
        return;
    }

    device_event_data_t event_data;
    event_data.text = task_copy;
    app_device_event_enqueue_with_data(DEVICE_EVENT_REMINDER, &event_data);

    free(task);
}

esp_err_t app_common_tools_set_reminder_handler(esp_agent_handle_t handle, const char *tool_name,
                                                esp_agent_tool_param_t params[], size_t num_params, void *user_data,
                                                char **result)
{
    const char *task = NULL;
    int timeout = 0;
    esp_err_t ret = ESP_OK;
    char *task_copy = NULL;
    esp_timer_handle_t timer_handle = NULL;

    for (size_t i = 0; i < num_params; i++) {
        if (strcmp(params[i].name, "task") == 0 && params[i].type == ESP_AGENT_PARAM_TYPE_STRING) {
            task = params[i].value.s;
        } else if (strcmp(params[i].name, "timeout") == 0 && params[i].type == ESP_AGENT_PARAM_TYPE_INT) {
            timeout = params[i].value.i;
        }
    }

    if (!task || timeout <= 0) {
        *result =
            strdup("Error: Invalid parameters. 'task' must be a string and 'timeout' must be a positive integer.");
        ret = ESP_ERR_INVALID_ARG;
        goto err;
    }

    task_copy = strdup(task);
    if (!task_copy) {
        *result = strdup("Error: Failed to allocate memory for task string.");
        ret = ESP_ERR_NO_MEM;
        goto err;
    }

    esp_timer_create_args_t timer_args = {
        .callback = reminder_timer_callback, .arg = task_copy, .name = "reminder_timer"};

    ret = esp_timer_create(&timer_args, &timer_handle);
    if (ret != ESP_OK) {
        free(task_copy);
        *result = strdup("Error: Failed to create timer.");
        goto err;
    }

    ret = esp_timer_start_once(timer_handle, timeout * 1000000ULL);
    if (ret != ESP_OK) {
        *result = strdup("Error: Failed to start timer.");
        goto err;
    }

    char *success_msg = malloc(256);
    if (!success_msg) {
        *result = strdup("Error: Failed to allocate memory for response.");
        ret = ESP_ERR_NO_MEM;
        goto err;
    }

    snprintf(success_msg, 256, "Reminder set for '%s' in %d seconds.", task, timeout);
    *result = success_msg;

    ESP_LOGI(TAG, "Reminder set: %s (timeout: %d seconds)", task, timeout);

    return ESP_OK;
err:
    if (task_copy) {
        free(task_copy);
    }
    if (timer_handle) {
        esp_timer_delete(timer_handle);
    }
    return ret;
}

esp_err_t app_common_tools_get_local_time_handler(esp_agent_handle_t handle, const char *tool_name,
                                                  esp_agent_tool_param_t params[], size_t num_params, void *user_data,
                                                  char **result)
{
    esp_err_t err = ESP_OK;
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    if (tm == NULL) {
        *result = strdup("Error: Failed to get current time.");
        err = ESP_ERR_INVALID_STATE;
        return err;
    }

    char *time_str = malloc(256);
    if (time_str == NULL) {
        *result = strdup("Error: Failed to allocate memory for time string.");
        err = ESP_ERR_NO_MEM;
        return err;
    }

    strftime(time_str, 256, "%Y-%m-%d %H:%M:%S", tm);
    ESP_LOGI(TAG, "Sending current time: %s", time_str);
    *result = time_str;
    return ESP_OK;
}

esp_err_t app_common_tools_set_volume_handler(esp_agent_handle_t handle, const char *tool_name,
                                              esp_agent_tool_param_t params[], size_t num_params, void *user_data,
                                              char **result)
{
    int volume = -1;
    esp_err_t err = ESP_OK;
    for (size_t i = 0; i < num_params; i++) {
        if (strncmp(params[i].name, "volume", strlen("volume")) == 0 && params[i].type == ESP_AGENT_PARAM_TYPE_INT) {
            volume = params[i].value.i;
        }
    }
    if (volume == -1) {
        *result = strdup("Volume parameter not found.");
        err = ESP_ERR_INVALID_ARG;
        return err;
    }

    err = app_audio_set_playback_volume(volume);
    if (err == ESP_OK) {
        *result = strdup("Volume set successfully.");
    } else {
        *result = strdup("Failed to set volume.");
    }
    return err;
}
