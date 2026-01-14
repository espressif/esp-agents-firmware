/**
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <esp_err.h>

/**
 * @brief Register application-specific tools with the agent
 *
 * @note This function uses the internal agent handle from app_agent_init().
 *       The agent_handle parameter is kept for API compatibility but is unused.
 *
 * @param agent_handle Agent handle (unused, kept for API compatibility)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t app_tools_register(void);
