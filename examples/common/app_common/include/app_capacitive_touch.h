/**
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <esp_err.h>

/**
 * @brief Initialize the touch sensor if supported
 *
 * @param on_active_callback The callback function for touch active event
 * @param on_inactive_callback The callback function for touch inactive event
 *
 * @return ESP_OK on success or not supported, otherwise an error code
 */
esp_err_t app_capacitive_touch_init();
