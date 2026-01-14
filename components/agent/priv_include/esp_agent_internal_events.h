/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <esp_agent.h>

#include <esp_event.h>

#include <esp_agent_internal.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Post an event to the agent's event loop
 *
 * @param handle Agent handle
 * @param event Event type
 * @param data Event data
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t esp_agent_post_event(esp_agent_handle_t handle, esp_agent_event_t event, esp_agent_message_data_t *data);

/**
 * @brief Internal event handler for cleanup
 *
 * This should always be the last event handler in the chain.
 *
 * @param handler_args Handler arguments
 * @param base Event base
 * @param event_id Event ID
 * @param event_data Event data
 */
void esp_agent_internal_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);

#ifdef __cplusplus
}
#endif
