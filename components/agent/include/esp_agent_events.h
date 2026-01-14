/**
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <esp_event.h>

#include "esp_agent_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Agent event types, received by the event handler.
 *
 * @note Text and Speech data received from the server are sent as events.
 */
typedef enum {
    ESP_AGENT_EVENT_INIT,
    ESP_AGENT_EVENT_DEINIT,
    ESP_AGENT_EVENT_START,
    ESP_AGENT_EVENT_STOP,

    ESP_AGENT_EVENT_ERROR,

    ESP_AGENT_EVENT_CONNECTED,
    ESP_AGENT_EVENT_DISCONNECTED,

    ESP_AGENT_EVENT_SPEECH_START,
    ESP_AGENT_EVENT_SPEECH_END,

    ESP_AGENT_EVENT_DATA_TYPE_TEXT,
    ESP_AGENT_EVENT_DATA_TYPE_THINKING,
    ESP_AGENT_EVENT_DATA_TYPE_SPEECH,

    ESP_AGENT_EVENT_DATA_TYPE_MAX,
} esp_agent_event_t;

/**
 * @brief Error codes, received by the event handler.
 */
typedef enum {
    ESP_AGENT_AUDIO_CONVERSATION_ERROR,
    ESP_AGENT_ERROR_MAX,
} esp_agent_error_t;

/**
 * @brief Message roles, for text data.
 */
typedef enum {
    ESP_AGENT_MESSAGE_ROLE_USER,
    ESP_AGENT_MESSAGE_ROLE_ASSISTANT,
    ESP_AGENT_MESSAGE_ROLE_MAX,
} esp_agent_message_role_t;

/**
 * @brief Message generation stages, for text data(only applicable for assistant messages).
 */
typedef enum {
    ESP_AGENT_MESSAGE_GENERATION_STAGE_SPECULATIVE,
    ESP_AGENT_MESSAGE_GENERATION_STAGE_FINAL,
    ESP_AGENT_MESSAGE_GENERATION_STAGE_UNKNOWN,
} esp_agent_message_generation_stage_t;

/**
 * @brief This is the `event_data` for the event handler, based on the type of event.
 */
typedef union {
    struct {
        const char *text;
        esp_agent_message_role_t role;
        esp_agent_message_generation_stage_t generation_stage;
    } text;

    struct {
        const uint8_t *data;
        const size_t len;
    } speech;

    struct {
        const char *conversation_id;
    } start;

    struct {
        const char *thought;
    } thinking;

    struct {
        esp_agent_error_t error;
    } error;
} esp_agent_message_data_t;

/**
 * @brief This registers the events handler for the agent.
 *
 * @param[in] handle Agent handle obtained from esp_agent_init
 * @param[in] event Event type to register handler for
 * @param[in] handler Event handler function pointer
 * @param[in] user_data User data passed to the event handler
 * @param[out] handler_instance Pointer to the event handler instance
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t esp_agent_register_event_handler(esp_agent_handle_t handle, esp_agent_event_t event, esp_event_handler_t handler, void *user_data, esp_event_handler_instance_t *handler_instance);

/**
 * @brief This unregisters the events handler for the agent.
 *
 * @param[in] handle Agent handle obtained from esp_agent_init
 * @param[in] handler_instance Pointer to the event handler instance
 * @param[in] event Event type to unregister handler for
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t esp_agent_unregister_event_handler(esp_agent_handle_t handle, esp_event_handler_instance_t *handler_instance, esp_agent_event_t event);

#ifdef __cplusplus
}
#endif
