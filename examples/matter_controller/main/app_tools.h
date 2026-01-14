/**
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <esp_agent.h>

/**
 * @brief Register application-specific tools with the agent
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t app_tools_register();
