/**
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <esp_check.h>
#include <esp_log.h>

#include "app_tools.h"
#include "app_common_tools.h"
#include "app_agent.h"
#include "app_display.h"
#include <string.h>

static const char *TAG = "app_agent_tools";

static esp_err_t app_tools_set_emotion_handler(esp_agent_handle_t handle, const char *tool_name,
                                               esp_agent_tool_param_t params[], size_t num_params, void *user_data,
                                               char **result)
{
    const char *emotion = NULL;
    esp_err_t err = ESP_OK;
    for (size_t i = 0; i < num_params; i++) {
        if (strncmp(params[i].name, "emotion_name", strlen("emotion_name")) == 0 &&
            params[i].type == ESP_AGENT_PARAM_TYPE_STRING) {
            emotion = params[i].value.s;
        }
    }
    if (emotion == NULL) {
        *result = strdup("Emotion parameter not found.");
        err = ESP_ERR_INVALID_ARG;
        return err;
    }

    /* Check if LLM has provided a valid emotion name */
    if (!app_display_is_emotion_valid(emotion)) {
        *result = strdup("Invalid emotion.");
        err = ESP_ERR_INVALID_ARG;
        return err;
    }

    ESP_LOGI(TAG, "Setting emotion: %s", emotion);
    err = app_display_set_emotion(emotion);
    if (err == ESP_ERR_INVALID_ARG) {
        *result = strdup("Invalid emotion.");
    } else if (err != ESP_OK) {
        *result = strdup("Failed to set emotion.");
    }
    return err;
}

esp_err_t app_tools_register(void)
{
    /* Register common tools */
    app_agent_register_tool(TOOL_NAME_SET_REMINDER, app_common_tools_set_reminder_handler, NULL);
    app_agent_register_tool(TOOL_NAME_GET_LOCAL_TIME, app_common_tools_get_local_time_handler, NULL);
    app_agent_register_tool(TOOL_NAME_SET_VOLUME, app_common_tools_set_volume_handler, NULL);

    /* Register voice_chat specific tools */
    app_agent_register_tool("set_emotion", app_tools_set_emotion_handler, NULL);

    return ESP_OK;
}
