/**
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <esp_check.h>
#include <esp_log.h>

#include "app_tools.h"
#include "app_common_tools.h"
#include "app_agent.h"
#include "app_controller.h"
#include "app_display.h"

static const char *TAG = "app_agent_tools";

#define ERROR_CHECK_WARN(err, msg) \
    if (err != ESP_OK) { \
        ESP_LOGW(TAG, "%s: %s", msg, esp_err_to_name(err)); \
    }

static esp_err_t app_tools_get_device_list_handler(esp_agent_handle_t handle, const char *tool_name,
                                                   esp_agent_tool_param_t params[], size_t num_params, void *user_data,
                                                   char **result)
{
    char *device_list = NULL;
    matter_controller_get_device_list(&device_list);

    if (device_list) {
        *result = strdup(device_list);
        free(device_list);
    } else {
        *result = strdup("Failed to get device list.");
    }
    return ESP_OK;
}

static esp_err_t app_tools_control_device_handler(esp_agent_handle_t handle, const char *tool_name,
                                                  esp_agent_tool_param_t params[], size_t num_params, void *user_data,
                                                  char **result)
{
    const char *node_id = NULL;
    uint32_t cluster_id = 0;
    uint32_t command_id = 0;
    const char *command_params_json = NULL;

    for (size_t i = 0; i < num_params; i++) {
        if (strcmp(params[i].name, "node_id") == 0 && params[i].type == ESP_AGENT_PARAM_TYPE_STRING) {
            node_id = params[i].value.s;
        } else if (strcmp(params[i].name, "cluster_id") == 0 && params[i].type == ESP_AGENT_PARAM_TYPE_INT) {
            cluster_id = params[i].value.i;
        } else if (strcmp(params[i].name, "command_id") == 0 && params[i].type == ESP_AGENT_PARAM_TYPE_INT) {
            command_id = params[i].value.i;
        } else if (strcmp(params[i].name, "command_args") == 0 && params[i].type == ESP_AGENT_PARAM_TYPE_STRING) {
            command_params_json = params[i].value.s;
        }
    }

    if (!node_id || cluster_id == 0) {
        *result =
            strdup("Error: Invalid parameters. 'node_id' must be a string, 'cluster_id' must be positive integers.");
        return ESP_ERR_INVALID_ARG;
    }

    char *command_result = NULL;
    matter_controller_control_device(&command_result, strtoull(node_id, NULL, 16), cluster_id, command_id,
                                     command_params_json);

    if (command_result) {
        *result = strdup(command_result);
        free(command_result);
    } else {
        *result = strdup("Failed to control device.");
    }
    return ESP_OK;
}

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

    /* Register Matter controller specific tools */
    app_agent_register_tool("get_device_list", app_tools_get_device_list_handler, NULL);
    app_agent_register_tool("control_device", app_tools_control_device_handler, NULL);
    app_agent_register_tool("set_emotion", app_tools_set_emotion_handler, NULL);

    return ESP_OK;
}
