/**
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <esp_err.h>
#include <esp_gmf_pool.h>

#pragma once

/**
 *
 * This function initializes the GMF audio pool.
 * It allocates memory for the audio pool and initializes common required elements in the pool.
 *
 * @return ESP_OK on success, otherwise an error code
 */
esp_err_t audio_pool_setup(void);

/**
 *
 * This function returns the GMF audio pool handle.
 * It returns the handle to the GMF audio pool.
 *
 * @return GMF audio pool handle
 */
esp_gmf_pool_handle_t audio_pool_get(void);

/**
 * This function registers AFE (Audio Front End) elements to the shared pool.
 * It sets up the AFE manager and registers the ai_afe element.
 *
 * @param input_format The input format string for AFE configuration
 * @return AFE manager handle on success, NULL on failure
 */
void* audio_pool_register_afe(const char *input_format);
