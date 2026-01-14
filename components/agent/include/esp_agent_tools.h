/**
 * @file
 * @brief ESP Agent Tools API
 *
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdbool.h>

#include <esp_err.h>

#include "esp_agent_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Currently only supporting int, string and bool
 */
typedef enum {
    ESP_AGENT_PARAM_TYPE_INT,
    ESP_AGENT_PARAM_TYPE_STRING,
    ESP_AGENT_PARAM_TYPE_BOOL,
    ESP_AGENT_PARAM_TYPE_MAX,
} esp_agent_tool_param_type_t;

/**
 * @brief Value for the function call
 */
typedef union {
    int i;
    const char *s;
    bool b;
} esp_agent_tool_param_value_t;

/**
 * @brief Tool parameter structure
 */
typedef struct {
    const char *name;
    esp_agent_tool_param_type_t type;
    esp_agent_tool_param_value_t value;
} esp_agent_tool_param_t;

/**
 * @brief Function callback signature
 * @param[in] handle Agent handle
 * @param[in] tool_name Name of the tool being executed
 * @param[in] params Array of tool parameters
 * @param[in] num_params Number of tool parameters
 * @param[in] user_data User data
 * @param[out] result Pointer to the result string
 * @return ESP_OK on success, error code otherwise
 *
 * @note The result string must heap allocated and will be freed internally after sending the tool response.
 */
typedef esp_err_t (*esp_agent_tool_handler_t)(esp_agent_handle_t handle, const char *tool_name, esp_agent_tool_param_t params[], size_t num_params, void *user_data, char **result);

/**
 * @brief Registers a local tool handler with the agent.
 *
 * @param[in] handle Agent handle obtained from esp_agent_init
 * @param[in] name Name of the tool to register
 * @param[in] tool_handler Function pointer to the tool handler
 * @param[in] user_data User data passed to the tool handler
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t esp_agent_register_local_tool(esp_agent_handle_t handle, const char *name, esp_agent_tool_handler_t tool_handler, void *user_data);

/**
 * @brief This unregisters the local tool for the agent.
 *
 * @param[in] handle Agent handle obtained from esp_agent_init
 * @param[in] name Name of the tool to unregister
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t esp_agent_unregister_local_tool(esp_agent_handle_t handle, const char *name);

#ifdef __cplusplus
}
#endif
