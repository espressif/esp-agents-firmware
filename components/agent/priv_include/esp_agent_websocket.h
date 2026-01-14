/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <esp_agent.h>
#include <freertos/FreeRTOS.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* WebSocket send message types */
typedef enum {
    WS_SEND_MSG_TYPE_TEXT,
    WS_SEND_MSG_TYPE_BINARY,
} ws_send_msg_type_t;

/* WebSocket send message structure */
typedef struct {
    ws_send_msg_type_t type;
    char *payload;
    size_t len;
} ws_send_message_t;

/**
 * @brief Start the WebSocket connection and authenticate
 *
 * @param handle Agent handle
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t esp_agent_websocket_start(esp_agent_handle_t handle);

/**
 * @brief Queue a message to be sent over WebSocket
 *
 * @param handle Agent handle
 * @param type Message type (text or binary)
 * @param payload Message payload
 * @param len Payload length
 * @param timeout Queue timeout
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t esp_agent_websocket_queue_message(esp_agent_handle_t handle, ws_send_msg_type_t type, const char *payload, size_t len, TickType_t timeout);

/**
 * @brief WebSocket send task
 *
 * @param pvParameters Agent handle pointer
 */
void esp_agent_websocket_send_task(void *pvParameters);

/**
 * @brief WebSocket event handler
 *
 * @param handler_args Agent handle pointer
 * @param base Event base
 * @param event_id Event ID
 * @param event_data Event data
 */
void esp_agent_websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);

#ifdef __cplusplus
}
#endif
