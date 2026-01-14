/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <esp_err.h>
#include <agent_setup.h>

/* @brief Initialize the RainMaker setup
 *
 * @return ESP_OK on success, otherwise an error code
 */
 esp_err_t setup_rainmaker_init(const char *board_device_manual_url);

/**
 * @brief factory reset the device
 *
 * @return ESP_OK if success, error otherwise
 */
esp_err_t setup_rainmaker_factory_reset(void);

/**
 * @brief Register callbacks for volume get/set operations
 *
 * @param get_cb Callback function to get current volume (0-100)
 * @param set_cb Callback function to set volume (0-100)
 * @return ESP_OK on success, otherwise an error code
 */
esp_err_t setup_rainmaker_register_volume_callbacks(esp_err_t (*get_cb)(uint8_t *volume), esp_err_t (*set_cb)(uint8_t volume));

/**
 * @brief Update the volume parameter value in RainMaker
 *
 * @param volume Volume value (0-100)
 * @return ESP_OK on success, otherwise an error code
 */
esp_err_t setup_rainmaker_update_volume(uint8_t volume);
