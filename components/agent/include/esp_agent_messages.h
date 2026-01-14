/**
 * @file
 * @brief ESP Agent Messages API for speech and text operations
 *
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <freertos/FreeRTOS.h>
#include <stdint.h>

#include <esp_err.h>

#include "esp_agent_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief This will start a new speech conversation.
 *
 * @param[in] handle Agent handle obtained from esp_agent_init
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t esp_agent_speech_conversation_start(esp_agent_handle_t handle);

/**
 * @brief This will end the current speech conversation.
 *
 * @param[in] handle Agent handle obtained from esp_agent_init
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t esp_agent_speech_conversation_end(esp_agent_handle_t handle);

/**
 * @brief This sends the speech data to the server
 *
 * @param[in] handle Agent handle obtained from esp_agent_init
 * @param[in] data Pointer to speech data buffer
 * @param[in] len Length of speech data
 * @param[in] timeout Timeout for sending operation
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t esp_agent_send_speech(esp_agent_handle_t handle, const uint8_t *data, size_t len, TickType_t timeout);

/**
 * @brief This sends the text data to the server
 *
 * @param[in] handle Agent handle obtained from esp_agent_init
 * @param[in] text Text data to send(should be NULL terminated)
 * @param[in] timeout Timeout for sending operation
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t esp_agent_send_text(esp_agent_handle_t handle, const char *text, TickType_t timeout);

#ifdef __cplusplus
}
#endif
