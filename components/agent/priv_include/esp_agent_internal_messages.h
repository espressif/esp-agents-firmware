/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cJSON.h>

#include <esp_agent.h>

#include <esp_agent_internal.h>

#define ESP_AGENT_MESSAGE_TYPE_HANDSHAKE "handshake"
#define ESP_AGENT_MESSAGE_TYPE_HANDSHAKE_ACK "handshake_ack"
#define ESP_AGENT_MESSAGE_TYPE_AUDIO_STREAM_START "audio_stream_start"
#define ESP_AGENT_MESSAGE_TYPE_AUDIO_STREAM_END "audio_stream_end"
#define ESP_AGENT_MESSAGE_TYPE_TRANSACTION_END "transaction_end"
#define ESP_AGENT_MESSAGE_TYPE_BARGE_IN "barge_in"
#define ESP_AGENT_MESSAGE_TYPE_USAGE_INFO "usage_info"
#define ESP_AGENT_MESSAGE_TYPE_USER "user"
#define ESP_AGENT_MESSAGE_TYPE_ASSISTANT "assistant"
#define ESP_AGENT_MESSAGE_TYPE_THINKING "thinking"
#define ESP_AGENT_MESSAGE_TYPE_ERROR "error"
#define ESP_AGENT_MESSAGE_TYPE_TOOL_CALL_INFO "tool_call_info"
#define ESP_AGENT_MESSAGE_TYPE_TOOL_REQUEST "tool_request"
#define ESP_AGENT_MESSAGE_TYPE_TOOL_RESPONSE "tool_response"
#define ESP_AGENT_MESSAGE_TYPE_TOOL_RESULT_INFO "tool_result_info"

typedef esp_err_t (*esp_agent_message_handler_t)(esp_agent_handle_t handle, cJSON *content, cJSON *metadata);

typedef struct {
    const char *type;
    esp_agent_message_handler_t handler;
} esp_agent_message_handler_info_t;

/**
 * @brief Parse the message content, and invoke appropriate handler
 *
 * @param message The message to parse
 * @return ESP_OK if the message is parsed successfully, otherwise an error code
 */
esp_err_t esp_agent_messages_parse_process(esp_agent_handle_t handle, char *message);

/**
 * @brief Get the handshake message string for the agent
 *
 * @param handle The agent handle
 * @return The JSON string of the handshake message, if successful, otherwise NULL
 */
char *esp_agent_messages_get_handshake(esp_agent_handle_t handle);

/**
 * @brief Prepare the tool response message
 *
 * @param handle The agent handle
 * @param request_id The request ID
 * @param result The result of the tool execution
 * @return The JSON string of the tool response message, if successful, otherwise NULL
 */
char *esp_agent_messages_prepare_tool_response(esp_agent_handle_t handle, char *request_id, esp_err_t status, char *tool_result);

/**
 * @brief Prepare the text message for the agent
 *
 * @param handle The agent handle
 * @param msg The message content
 * @return The JSON string of the text message, if successful, otherwise NULL
 */
char *esp_agent_messages_prepare_text(esp_agent_handle_t handle, const char *msg);
/**
 * @brief Prepare the speech conversation start message
 *
 * @param handle The agent handle
 * @return The JSON string of the speech conversation start message, if successful, otherwise NULL
 */
char *esp_agent_messages_prepare_speech_conversation_start(esp_agent_handle_t handle);

/**
 * @brief Prepare the speech conversation end message
 *
 * @param handle The agent handle
 * @return The JSON string of the speech conversation end message, if successful, otherwise NULL
 */
char *esp_agent_messages_prepare_speech_conversation_end(esp_agent_handle_t handle);
