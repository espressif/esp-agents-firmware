/**
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <esp_agent.h>

// Expose agent state enum
typedef enum {
    APP_AGENT_STATE_DISCONNECTED = 0,
    APP_AGENT_STATE_CONNECTING,
    APP_AGENT_STATE_CONNECTED,
    APP_AGENT_STATE_STARTED,
} app_agent_state_t;

typedef struct {
    /* Custom event handler for agent events.
     * Applications can provide their own event handler to customize behavior.
     */
    esp_event_handler_t event_handler;

} app_agent_config_t;

esp_err_t app_agent_init(app_agent_config_t *config);

esp_err_t app_agent_start(void);

esp_err_t app_agent_connect(void);

esp_err_t app_agent_send_speech(uint8_t *audio_data, size_t audio_data_len);

bool app_agent_is_active(void);

app_agent_state_t app_agent_get_state(void);

esp_err_t app_agent_speech_conversation_start(void);

esp_err_t app_agent_speech_conversation_end(void);

void app_agent_default_event_handler(void *arg, esp_event_base_t event_base,
                                     int32_t event_id, void *event_data);

/**
 * @brief Register a local tool with the agent
 *
 * @param[in] name Name of the tool
 * @param[in] tool_handler Function pointer to the tool handler
 * @param[in] user_data User data passed to the tool handler
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t app_agent_register_tool(const char *name, esp_agent_tool_handler_t tool_handler, void *user_data);

/**
 * @brief Unregister a local tool from the agent
 *
 * @param[in] name Name of the tool to unregister
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t app_agent_tool_unregister(const char *name);
