/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <esp_agent.h>

#include <esp_agent_internal.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Execute a client tool (called from message handler)
 *
 * @param handle Agent handle
 * @param request_id Request ID for the tool call
 * @param tool_name Name of the tool to execute
 * @param parameters Array of tool parameters
 * @param num_parameters Number of parameters
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t esp_agent_execute_tool(esp_agent_handle_t handle, char *request_id, char *tool_name, esp_agent_tool_param_t *parameters, size_t num_parameters);

#ifdef __cplusplus
}
#endif
