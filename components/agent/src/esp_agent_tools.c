/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <string.h>
#include <stdlib.h>

#include <esp_log.h>
#include <esp_check.h>

#include <esp_agent.h>
#include <esp_agent_internal_tools.h>
#include <esp_agent_websocket.h>
#include <esp_agent_internal_messages.h>

static const char *TAG = "esp_agent_tools";

typedef struct {
    char *request_id;
    char *tool_name;
    esp_agent_tool_param_t *parameters;
    size_t num_parameters;
    esp_agent_tool_handler_t tool_handler;
    void *user_data;
    esp_agent_handle_t handle;
} tool_request_t;

static void execute_tool_task(void *pvParameters)
{
    tool_request_t *request = (tool_request_t *)pvParameters;
    if (request == NULL) {
        ESP_LOGE(TAG, "Invalid request");
        vTaskDelete(NULL);
        return;
    }

    esp_agent_t *agent = (esp_agent_t *)request->handle;
    char *tool_result = NULL;
    ESP_LOGD(TAG, "Executing tool: %s", request->tool_name);
    esp_err_t err = request->tool_handler(agent, request->tool_name, request->parameters, request->num_parameters, request->user_data, &tool_result);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to execute tool: 0x%x", err);
    }
    char *tool_response_json_str = esp_agent_messages_prepare_tool_response(agent, request->request_id, err, tool_result);
    if (tool_response_json_str == NULL) {
        ESP_LOGE(TAG, "Failed to prepare tool response");
        goto end;
    }
    ESP_LOGD(TAG, "Tool response: %s", tool_response_json_str);

    esp_err_t queue_err = esp_agent_websocket_queue_message(agent, WS_SEND_MSG_TYPE_TEXT, tool_response_json_str, strlen(tool_response_json_str), portMAX_DELAY);
    if (queue_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to queue tool response: %d", queue_err);
    }
    free(tool_response_json_str);

end:
    if (request->request_id) {
        free(request->request_id);
    }
    if (request->tool_name) {
        free(request->tool_name);
    }
    if (request->parameters) {
        for (size_t i = 0; i < request->num_parameters; i++) {
            if (request->parameters[i].name) {
                free((char *)request->parameters[i].name);
            }
            if (request->parameters[i].type == ESP_AGENT_PARAM_TYPE_STRING && request->parameters[i].value.s) {
                free((char *)request->parameters[i].value.s);
            }
        }
        free(request->parameters);
    }
    if (tool_result) {
        free(tool_result);
    }
    free(request);
    vTaskDelete(NULL);
}

esp_err_t esp_agent_execute_tool(esp_agent_handle_t handle, char *request_id, char *tool_name, esp_agent_tool_param_t *parameters, size_t num_parameters)
{
    if (handle == NULL || tool_name == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_agent_t *agent = (esp_agent_t *)handle;
    local_tool_node_t *tool_node = agent->local_tools;
    while (tool_node != NULL) {
        if (strcmp(tool_node->name, tool_name) == 0) {
            ESP_LOGD(TAG, "Found tool: %s", tool_name);
            tool_request_t *request = malloc(sizeof(tool_request_t));
            if (request == NULL) {
                ESP_LOGE(TAG, "Failed to allocate memory for tool request");
                return ESP_ERR_NO_MEM;
            }
            request->request_id = strdup(request_id);
            request->tool_name = strdup(tool_name);
            request->parameters = parameters;
            request->num_parameters = num_parameters;
            request->tool_handler = tool_node->tool_handler;
            request->user_data = tool_node->user_data;
            request->handle = handle;

            xTaskCreate(execute_tool_task, "execute_tool_task", 4096, request, 5, NULL);
            return ESP_OK;
        }
        tool_node = tool_node->next;
    }
    ESP_LOGE(TAG, "Tool with name '%s' not found", tool_name);
    return ESP_ERR_NOT_FOUND;
}

esp_err_t esp_agent_register_local_tool(esp_agent_handle_t handle, const char *name, esp_agent_tool_handler_t tool_handler, void *user_data)
{
    if (handle == NULL || name == NULL || tool_handler == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_agent_t *agent = (esp_agent_t *)handle;

    // Check for duplicate tool names
    local_tool_node_t *existing_node = agent->local_tools;
    while (existing_node != NULL) {
        if (strcmp(existing_node->name, name) == 0) {
            ESP_LOGE(TAG, "Tool with name '%s' already registered", name);
            return ESP_ERR_INVALID_STATE;
        }
        existing_node = existing_node->next;
    }

    local_tool_node_t *new_node = malloc(sizeof(local_tool_node_t));
    if (new_node == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for tool node");
        return ESP_ERR_NO_MEM;
    }

    new_node->name = strdup(name);
    if (new_node->name == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for tool name");
        free(new_node);
        return ESP_ERR_NO_MEM;
    }

    new_node->tool_handler = tool_handler;
    new_node->user_data = user_data;

    new_node->next = agent->local_tools;
    agent->local_tools = new_node;

    ESP_LOGI(TAG, "Registered local tool: %s", name);
    return ESP_OK;
}

esp_err_t esp_agent_unregister_local_tool(esp_agent_handle_t handle, const char *name)
{
    if (handle == NULL || name == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_agent_t *agent = (esp_agent_t *)handle;

    // Find the tool node by name
    local_tool_node_t *tool_node = agent->local_tools;
    local_tool_node_t *prev_node = NULL;

    while (tool_node != NULL) {
        if (strcmp(tool_node->name, name) == 0) {
            if (prev_node == NULL) {
                agent->local_tools = tool_node->next;
            } else {
                prev_node->next = tool_node->next;
            }

            if (tool_node->name) {
                free(tool_node->name);
            }
            free(tool_node);

            ESP_LOGI(TAG, "Unregistered local tool: %s", name);
            return ESP_OK;
        }
        prev_node = tool_node;
        tool_node = tool_node->next;
    }

    ESP_LOGW(TAG, "Tool with name '%s' not found", name);
    return ESP_ERR_NOT_FOUND;
}
