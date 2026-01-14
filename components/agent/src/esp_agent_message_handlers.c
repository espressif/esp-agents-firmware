/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <esp_log.h>
#include <cJSON.h>

#include <esp_agent.h>
#include <esp_agent_internal_messages.h>
#include <esp_agent_internal_events.h>
#include <esp_agent_internal_tools.h>

static const char *TAG = "esp_agent_message_handlers";

esp_err_t esp_agent_message_handshake_ack_handler(esp_agent_handle_t handle, cJSON *content, cJSON *metadata);
esp_err_t esp_agent_message_dummy_handler(esp_agent_handle_t handle, cJSON *content, cJSON *metadata);
esp_err_t esp_agent_message_transcript_handler(esp_agent_handle_t handle, cJSON *content, cJSON *metadata);
esp_err_t esp_agent_message_error_handler(esp_agent_handle_t handle, cJSON *content, cJSON *metadata);
esp_err_t esp_agent_message_audio_stream_start_handler(esp_agent_handle_t handle, cJSON *content, cJSON *metadata);
esp_err_t esp_agent_message_audio_stream_end_handler(esp_agent_handle_t handle, cJSON *content, cJSON *metadata);
esp_err_t esp_agent_message_tool_request_handler(esp_agent_handle_t handle, cJSON *content, cJSON *metadata);
esp_err_t esp_agent_message_thinking_handler(esp_agent_handle_t handle, cJSON *content, cJSON *metadata);

const esp_agent_message_handler_info_t esp_agent_message_handlers[] = {
    {.type = ESP_AGENT_MESSAGE_TYPE_HANDSHAKE_ACK, .handler = esp_agent_message_handshake_ack_handler},
    {.type = ESP_AGENT_MESSAGE_TYPE_USER, .handler = esp_agent_message_transcript_handler},
    {.type = ESP_AGENT_MESSAGE_TYPE_ASSISTANT, .handler = esp_agent_message_transcript_handler},
    {.type = ESP_AGENT_MESSAGE_TYPE_THINKING, .handler = esp_agent_message_thinking_handler},
    {.type = ESP_AGENT_MESSAGE_TYPE_ERROR, .handler = esp_agent_message_error_handler},
    {.type = ESP_AGENT_MESSAGE_TYPE_AUDIO_STREAM_START, .handler = esp_agent_message_audio_stream_start_handler},
    {.type = ESP_AGENT_MESSAGE_TYPE_AUDIO_STREAM_END, .handler = esp_agent_message_audio_stream_end_handler},
    {.type = ESP_AGENT_MESSAGE_TYPE_USAGE_INFO, .handler = esp_agent_message_dummy_handler},
    {.type = ESP_AGENT_MESSAGE_TYPE_TOOL_CALL_INFO, .handler = esp_agent_message_dummy_handler},
    {.type = ESP_AGENT_MESSAGE_TYPE_TOOL_REQUEST, .handler = esp_agent_message_tool_request_handler},
    {.type = ESP_AGENT_MESSAGE_TYPE_TOOL_RESULT_INFO, .handler = esp_agent_message_dummy_handler},
    {.type = ESP_AGENT_MESSAGE_TYPE_TRANSACTION_END, .handler = esp_agent_message_dummy_handler},
    {.type = ESP_AGENT_MESSAGE_TYPE_BARGE_IN, .handler = esp_agent_message_dummy_handler}
};
const size_t esp_agent_message_handlers_count = sizeof(esp_agent_message_handlers) / sizeof(esp_agent_message_handler_info_t);

esp_err_t esp_agent_message_handshake_ack_handler(esp_agent_handle_t handle, cJSON *content, cJSON *metadata)
{
    if (handle == NULL || content == NULL) {
        ESP_LOGE(TAG, "Invalid handle or content for processing handshake ack");
        ESP_LOGD(TAG, "handle: %p, content: %p", handle, content);
        return ESP_ERR_INVALID_ARG;
    }

    esp_agent_t *agent = (esp_agent_t *)handle;
    agent->handshake_state = ESP_AGENT_HANDSHAKE_DONE;

    cJSON *conversation_id = cJSON_GetObjectItemCaseSensitive(content, "conversationId");
    char *conv_id = cJSON_GetStringValue(conversation_id);
    if (!conv_id) {
        ESP_LOGE(TAG, "Failed to get Conversation ID");
        return ESP_FAIL;
    }

    ESP_LOGD(TAG, "Conversation: %s", conv_id);

    if (agent->conversation_id != NULL) {
        if (strcmp(agent->conversation_id, conv_id) != 0) {
            ESP_LOGW(TAG, "Received different conversation ID. Expected: %s, Got: %s",
                     agent->conversation_id, conv_id);
        }
        free(agent->conversation_id);
    }

    agent->conversation_id = strdup(conv_id);
    if (agent->conversation_id == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for conversation_id");
        return ESP_ERR_NO_MEM;
    }

    esp_agent_message_data_t event_data;
    event_data.start.conversation_id = strdup(conv_id);

    esp_err_t err = esp_agent_post_event(handle, ESP_AGENT_EVENT_START, &event_data);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to post start event");
        free((char *)event_data.start.conversation_id);
    }

    return err;
}

esp_err_t esp_agent_message_dummy_handler(esp_agent_handle_t handle, cJSON *content, cJSON *metadata)
{
    // NO-OP
    return ESP_OK;
}

esp_err_t esp_agent_message_transcript_handler(esp_agent_handle_t handle, cJSON *content, cJSON *metadata)
{
    if (handle == NULL || content == NULL) {
        ESP_LOGE(TAG, "Invalid handle or content for processing transcript");
        ESP_LOGD(TAG, "handle: %p, content: %p", handle, content);
        return ESP_ERR_INVALID_ARG;
    }

    char *content_str = cJSON_GetStringValue(content);
    if (!content_str) {
        ESP_LOGE(TAG, "Failed to get content string");
        return ESP_FAIL;
    }

    cJSON *role = cJSON_GetObjectItemCaseSensitive(metadata, "role");
    char *role_str = cJSON_GetStringValue(role);
    if (!role_str) {
        ESP_LOGE(TAG, "Failed to get role string");
        return ESP_FAIL;
    }

    cJSON *generation_stage = cJSON_GetObjectItemCaseSensitive(metadata, "generation_stage");
    char *generation_stage_str = cJSON_GetStringValue(generation_stage);

    esp_agent_message_data_t event_data;
    event_data.text.text = strdup(content_str);
    event_data.text.generation_stage = ESP_AGENT_MESSAGE_GENERATION_STAGE_UNKNOWN;

    if (strcmp(role_str, "user") == 0) {
        event_data.text.role = ESP_AGENT_MESSAGE_ROLE_USER;
    } else if (strcmp(role_str, "assistant") == 0) {
        event_data.text.role = ESP_AGENT_MESSAGE_ROLE_ASSISTANT;

        if (!generation_stage_str) {
            event_data.text.generation_stage = ESP_AGENT_MESSAGE_GENERATION_STAGE_UNKNOWN;
        } else if (strcmp(generation_stage_str, "speculative") == 0) {
            event_data.text.generation_stage = ESP_AGENT_MESSAGE_GENERATION_STAGE_SPECULATIVE;
        } else if (strcmp(generation_stage_str, "final") == 0) {
            event_data.text.generation_stage = ESP_AGENT_MESSAGE_GENERATION_STAGE_FINAL;
        }
    }


    esp_err_t err = esp_agent_post_event(handle, ESP_AGENT_EVENT_DATA_TYPE_TEXT, &event_data);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to post text event: 0x%x", err);
        free((char *)event_data.text.text);
    }
    return err;
}

esp_err_t esp_agent_message_thinking_handler(esp_agent_handle_t handle, cJSON *content, cJSON *metadata)
{
    if (handle == NULL || content == NULL) {
        ESP_LOGE(TAG, "Invalid handle or content for processing thinking");
        ESP_LOGD(TAG, "handle: %p, content: %p", handle, content);
        return ESP_ERR_INVALID_ARG;
    }
    char *thought = cJSON_GetStringValue(content);
    if (!thought) {
        ESP_LOGE(TAG, "Failed to get thought string");
        return ESP_FAIL;
    }

    esp_agent_message_data_t event_data;
    event_data.thinking.thought = strdup(thought);
    if (event_data.thinking.thought == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for agent thought");
        return ESP_ERR_NO_MEM;
    }
    esp_err_t err = esp_agent_post_event(handle, ESP_AGENT_EVENT_DATA_TYPE_THINKING, &event_data);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to post thinking event: 0x%x", err);
        free((char *)event_data.thinking.thought);
        return err;
    }
    return err;
}

esp_err_t esp_agent_message_error_handler(esp_agent_handle_t handle, cJSON *content, cJSON *metadata)
{
    if (content == NULL) {
        ESP_LOGE(TAG, "Invalid content for processing error");
        return ESP_ERR_INVALID_ARG;
    }

    esp_agent_message_data_t event_data;
    event_data.error.error = ESP_AGENT_ERROR_MAX;

    /* check if content is json */
    cJSON *json_content = cJSON_Parse(cJSON_GetStringValue(content));
    if (json_content != NULL) {
        cJSON *error_code = cJSON_GetObjectItemCaseSensitive(json_content, "code");
        char *error_code_str = cJSON_GetStringValue(error_code);
        if (error_code_str != NULL) {
            if (strcmp(error_code_str, "AUDIO_CONVERSATION_ERROR") == 0) {
                event_data.error.error = ESP_AGENT_AUDIO_CONVERSATION_ERROR;
            }
        }
        cJSON_Delete(json_content);
    }

    char *error_message = cJSON_PrintUnformatted(content);
    ESP_LOGE(TAG, "ESP Agent Error: %s", error_message);
    free(error_message);

    if (event_data.error.error != ESP_AGENT_ERROR_MAX) {
        esp_agent_post_event(handle, ESP_AGENT_EVENT_ERROR, &event_data);
    }

    return ESP_OK;
}

esp_err_t esp_agent_message_audio_stream_start_handler(esp_agent_handle_t handle, cJSON *content, cJSON *metadata)
{
    if (handle == NULL) {
        ESP_LOGE(TAG, "Invalid handle for processing audio stream start");
        return ESP_ERR_INVALID_ARG;
    }

    esp_agent_post_event(handle, ESP_AGENT_EVENT_SPEECH_START, NULL);
    return ESP_OK;
}

esp_err_t esp_agent_message_audio_stream_end_handler(esp_agent_handle_t handle, cJSON *content, cJSON *metadata)
{
    if (handle == NULL) {
        ESP_LOGE(TAG, "Invalid handle for processing audio stream end");
        return ESP_ERR_INVALID_ARG;
    }

    esp_agent_post_event(handle, ESP_AGENT_EVENT_SPEECH_END, NULL);
    return ESP_OK;
}

esp_err_t esp_agent_message_tool_request_handler(esp_agent_handle_t handle, cJSON *content, cJSON *metadata)
{
    if (handle == NULL || content == NULL) {
        ESP_LOGE(TAG, "Invalid handle or content for processing tool request");
        ESP_LOGD(TAG, "handle: %p, content: %p", handle, content);
        return ESP_ERR_INVALID_ARG;
    }

    char *request_id = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(content, "request_id"));
    char *tool_name = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(content, "tool_name"));
    cJSON *input = cJSON_GetObjectItemCaseSensitive(content, "input");

    if (!request_id || !tool_name || !input) {
        ESP_LOGE(TAG, "Failed to get tool request details");
        ESP_LOGD(TAG, "request_id: %p, tool_name: %p, input: %p", request_id, tool_name, input);
        return ESP_FAIL;
    }

    esp_agent_tool_param_t *parameters = NULL;
    size_t num_parameters = 0;

    num_parameters = cJSON_GetArraySize(input);
    if (num_parameters == 0) {
        ESP_LOGD(TAG, "No parameters found for tool: %s", tool_name);
        goto no_parameters;
    }

    parameters = (esp_agent_tool_param_t *)calloc(num_parameters, sizeof(esp_agent_tool_param_t));
    if (!parameters) {
        ESP_LOGE(TAG, "Failed to allocate memory for parameters");
        return ESP_ERR_NO_MEM;
    }

    for (size_t i = 0; i < num_parameters; i++) {
        cJSON *curr_element = cJSON_GetArrayItem(input, i);
        char *name = strdup(curr_element->string);
        ESP_LOGD(TAG, "Got Parameter: %s", name);
        parameters[i].name = name;
        cJSON *value = cJSON_GetObjectItemCaseSensitive(input, name);
        if (cJSON_IsString(value)) {
            parameters[i].type = ESP_AGENT_PARAM_TYPE_STRING;
            parameters[i].value.s = strdup(cJSON_GetStringValue(value));
        } else if (cJSON_IsNumber(value)) {
            parameters[i].type = ESP_AGENT_PARAM_TYPE_INT;
            parameters[i].value.i = cJSON_GetNumberValue(value);
        } else if (cJSON_IsBool(value)) {
            parameters[i].type = ESP_AGENT_PARAM_TYPE_BOOL;
            parameters[i].value.b = cJSON_IsTrue(value);
        } else {
            ESP_LOGE(TAG, "Invalid parameter value type");
            goto err;
        }
    }

no_parameters:
    char *input_str = cJSON_PrintUnformatted(input);
    ESP_LOGI(TAG, "Executing tool: %s: %s", tool_name, input_str);
    free(input_str);

    esp_err_t err = esp_agent_execute_tool(handle, request_id, tool_name, parameters, num_parameters);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to execute tool: 0x%x", err);
        goto err;
    }

    return ESP_OK;

err:
    if (parameters) {
        for (size_t i = 0; i < num_parameters; i++) {
            if (parameters[i].name) {
                free((char *)parameters[i].name);
            }
            if (parameters[i].type == ESP_AGENT_PARAM_TYPE_STRING && parameters[i].value.s) {
                free((char *)parameters[i].value.s);
            }
        }
        free(parameters);
    }
    return ESP_FAIL;
}
