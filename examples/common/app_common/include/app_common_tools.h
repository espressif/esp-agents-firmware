/**
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <esp_agent.h>
#include <esp_err.h>

#define TOOL_NAME_SET_REMINDER "set_reminder"
#define TOOL_NAME_GET_LOCAL_TIME "get_local_time"
#define TOOL_NAME_SET_VOLUME "set_volume"

esp_err_t app_common_tools_set_reminder_handler(esp_agent_handle_t handle, const char *tool_name,
                                                esp_agent_tool_param_t params[], size_t num_params, void *user_data,
                                                char **result);

esp_err_t app_common_tools_get_local_time_handler(esp_agent_handle_t handle, const char *tool_name,
                                                  esp_agent_tool_param_t params[], size_t num_params, void *user_data,
                                                  char **result);

esp_err_t app_common_tools_set_volume_handler(esp_agent_handle_t handle, const char *tool_name,
                                              esp_agent_tool_param_t params[], size_t num_params, void *user_data,
                                              char **result);
