/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <freertos/FreeRTOS.h>
#include <string.h>
#include <stdlib.h>

#include <esp_check.h>
#include <esp_log.h>
#include <cJSON.h>

#include <esp_agent_internal_messages.h>
#include <esp_agent_websocket.h>

extern const esp_agent_message_handler_info_t esp_agent_message_handlers[];
extern size_t esp_agent_message_handlers_count;

static const char *TAG = "esp_agent_messages";

esp_err_t esp_agent_messages_parse_process(esp_agent_handle_t handle, char *message)
{
    if (!handle || !message) {
        ESP_LOGE(TAG, "Invalid handle or message");
        return ESP_ERR_INVALID_ARG;
    }

    cJSON *json = cJSON_Parse(message);
    if (json == NULL) {
        ESP_LOGE(TAG, "Failed to parse JSON: %s", message);
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = ESP_OK;
    cJSON *type = cJSON_GetObjectItem(json, "type");
    char *type_str = cJSON_GetStringValue(type);
    if (!type_str) {
        ESP_LOGE(TAG, "Failed to get message type");
        err = ESP_FAIL;
        goto end;
    }

    bool handler_found = false;

    /* Checking if these are present is the reponsility of the respective handlers */
    cJSON *content = cJSON_GetObjectItem(json, "content");
    cJSON *metadata = cJSON_GetObjectItem(json, "metadata");

    ESP_LOGD(TAG, "Message type: %s", type_str);

    for (size_t i = 0; i < esp_agent_message_handlers_count; i++) {
        if (strcmp(type_str, esp_agent_message_handlers[i].type) == 0) {
            err = esp_agent_message_handlers[i].handler(handle, content, metadata);
            handler_found = true;
        }
    }

    if (!handler_found) {
        ESP_LOGW(TAG, "Handler not found for message type: %s", type_str);
    }

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to process message: %s", type_str);
    }

end:
    cJSON_Delete(json);
    return err;
}

static inline const char *esp_agent_messages_get_audio_format_string(esp_agent_conversation_audio_format_t format)
{
    if (format == ESP_AGENT_CONVERSATION_AUDIO_FORMAT_OPUS) {
        return "audio/opus";
    } else if (format == ESP_AGENT_CONVERSATION_AUDIO_FORMAT_PCM) {
        return "audio/pcm";
    }
    return NULL;
}


static cJSON *esp_agent_messages_get_audio_configuration(esp_agent_handle_t handle)
{
    if (handle == NULL) {
        return NULL;
    }

    esp_agent_t *agent = (esp_agent_t *)handle;
    esp_err_t ret = ESP_OK;

    cJSON *audio_configuration = cJSON_CreateObject();

    const char *input_format_string = esp_agent_messages_get_audio_format_string(agent->upload_audio_config.format);
    const char *output_format_string = esp_agent_messages_get_audio_format_string(agent->download_audio_config.format);

    cJSON *audio_input_config = cJSON_CreateObject();
    cJSON *audio_output_config = cJSON_CreateObject();

    ESP_GOTO_ON_FALSE(agent->upload_audio_config.sample_rate != 0, ESP_ERR_INVALID_ARG, err, TAG, "Invalid input sample rate");
    ESP_GOTO_ON_FALSE(agent->download_audio_config.sample_rate != 0, ESP_ERR_INVALID_ARG, err, TAG, "Invalid output sample rate");
    ESP_GOTO_ON_FALSE(agent->upload_audio_config.frame_duration != 0, ESP_ERR_INVALID_ARG, err, TAG, "Invalid input frame duration");
    ESP_GOTO_ON_FALSE(agent->download_audio_config.frame_duration != 0, ESP_ERR_INVALID_ARG, err, TAG, "Invalid output frame duration");

    ESP_GOTO_ON_FALSE(input_format_string, ESP_ERR_NO_MEM, err, TAG, "Failed to get input format string");
    ESP_GOTO_ON_FALSE(output_format_string, ESP_ERR_NO_MEM, err, TAG, "Failed to get output format string");

    ESP_GOTO_ON_FALSE(audio_input_config, ESP_ERR_NO_MEM, err, TAG, "Failed to create input JSON");
    ESP_GOTO_ON_FALSE(audio_output_config, ESP_ERR_NO_MEM, err, TAG, "Failed to create output JSON");

    cJSON_AddStringToObject(audio_input_config, "format", input_format_string);
    cJSON_AddNumberToObject(audio_input_config, "sampleRate", agent->upload_audio_config.sample_rate);
    cJSON_AddNumberToObject(audio_input_config, "frameDurationMs", agent->upload_audio_config.frame_duration);
    cJSON_AddItemToObject(audio_configuration, "input", audio_input_config);

    cJSON_AddStringToObject(audio_output_config, "format", output_format_string);
    cJSON_AddNumberToObject(audio_output_config, "sampleRate", agent->download_audio_config.sample_rate);
    cJSON_AddNumberToObject(audio_output_config, "frameDurationMs", agent->download_audio_config.frame_duration);
    cJSON_AddItemToObject(audio_configuration, "output", audio_output_config);

    /* For keping the compiler happy about not using variable ret */
    /* Will be optimized away */
    if (ret) {}

    /* Will be freed by the caller */
    return audio_configuration;

err:
    if (audio_input_config) {
        cJSON_Delete(audio_input_config);
    }
    if (audio_output_config) {
        cJSON_Delete(audio_output_config);
    }
    if (audio_configuration) {
        cJSON_Delete(audio_configuration);
    }
    return NULL;
}

char *esp_agent_messages_get_handshake(esp_agent_handle_t handle)
{
    if (handle == NULL) {
        return NULL;
    }

    esp_agent_t *agent = (esp_agent_t *)handle;
    char *handshake_json_str = NULL;
    esp_err_t ret = ESP_OK;

    cJSON *handshake_json = cJSON_CreateObject();
    cJSON *content = cJSON_CreateObject();

    cJSON *audio_configuration = NULL;

    bool audio_configuration_added_to_handshake = false; /* For calling cJSON_Delete appropriately */

    ESP_GOTO_ON_FALSE(handshake_json, ESP_ERR_NO_MEM, end, TAG, "Failed to create handshake JSON");
    ESP_GOTO_ON_FALSE(content, ESP_ERR_NO_MEM, end, TAG, "Failed to create content JSON");

    cJSON_AddStringToObject(handshake_json, "type", "handshake");

    if (agent->conversation_id) {
        cJSON_AddStringToObject(content, "conversationId", agent->conversation_id);
    }

    cJSON_AddStringToObject(content, "conversationType", agent->conversation_type == ESP_AGENT_CONVERSATION_SPEECH ? "audio" : "text");

    if (agent->conversation_type == ESP_AGENT_CONVERSATION_SPEECH) {
        audio_configuration = esp_agent_messages_get_audio_configuration(handle);
        ESP_GOTO_ON_FALSE(audio_configuration, ESP_ERR_NO_MEM, end, TAG, "Failed to get audio configuration");
        cJSON_AddItemToObject(content, "audioConfiguration", audio_configuration);
        audio_configuration_added_to_handshake = true;
    }

    cJSON_AddItemToObject(handshake_json, "content", content);
    cJSON_AddStringToObject(handshake_json, "content_type", "json");

    handshake_json_str = cJSON_PrintUnformatted(handshake_json);
    if (handshake_json_str == NULL) {
        ESP_LOGE(TAG, "Failed to print handshake JSON");
    }

    /* For keping the compiler happy about not using variable ret */
    /* Will be optimized away */
    if (ret) {}

end:
    if (!audio_configuration_added_to_handshake) {
        if (audio_configuration) {
            cJSON_Delete(audio_configuration);
        }
    }
    if (handshake_json) {
        cJSON_Delete(handshake_json);
    }

    return handshake_json_str;
}

char *esp_agent_messages_prepare_text(esp_agent_handle_t handle, const char *msg)
{
    if (handle == NULL || msg == NULL) {
        return NULL;
    }

    char *text_json_str = NULL;

    cJSON *text_json = cJSON_CreateObject();
    if (!text_json) {
        ESP_LOGE(TAG, "Failed to create JSON object");
        goto end;
    }

    cJSON_AddStringToObject(text_json, "type", "user");
    cJSON_AddStringToObject(text_json, "content_type", "text");
    cJSON_AddStringToObject(text_json, "content", msg);

    text_json_str = cJSON_PrintUnformatted(text_json);
    if (!text_json_str) {
        ESP_LOGE(TAG, "Failed to print text JSON");
    }

end:
    if (text_json) {
        cJSON_Delete(text_json);
    }
    return text_json_str;
}

char *esp_agent_messages_prepare_tool_response(esp_agent_handle_t handle, char *request_id, esp_err_t status, char *tool_result)
{
    if (handle == NULL || request_id == NULL) {
        return NULL;
    }

    esp_err_t ret = ESP_OK;

    cJSON *tool_response_json = cJSON_CreateObject();
    ESP_GOTO_ON_FALSE(tool_response_json, ESP_ERR_NO_MEM, err, TAG, "Failed to create tool response JSON");
    cJSON_AddStringToObject(tool_response_json, "type", ESP_AGENT_MESSAGE_TYPE_TOOL_RESPONSE);

    cJSON *content_type = cJSON_CreateObject();
    ESP_GOTO_ON_FALSE(content_type, ESP_ERR_NO_MEM, err, TAG, "Failed to create content type JSON");
    cJSON_AddStringToObject(content_type, "type", "json");
    cJSON_AddItemToObject(tool_response_json, "content_type", content_type);

    cJSON *result = cJSON_CreateObject();
    ESP_GOTO_ON_FALSE(result, ESP_ERR_NO_MEM, err, TAG, "Failed to create result JSON");
    cJSON_AddStringToObject(result, "status", status == ESP_OK ? "success" : "error");
    if (tool_result) {
        cJSON_AddStringToObject(result, "result", tool_result);
    }

    cJSON *content = cJSON_CreateObject();
    ESP_GOTO_ON_FALSE(content, ESP_ERR_NO_MEM, err, TAG, "Failed to create content JSON");
    cJSON_AddStringToObject(content, "request_id", request_id);
    cJSON_AddItemToObject(content, "result", result);
    cJSON_AddItemToObject(tool_response_json, "content", content);

    return cJSON_PrintUnformatted(tool_response_json);

    /* To prevent unused variable warning */
    if (0) {
        printf("%d", ret);
    }

err:
    if (tool_response_json) {
        cJSON_Delete(tool_response_json);
    }
    return NULL;
}

char *esp_agent_messages_prepare_speech_conversation_start(esp_agent_handle_t handle)
{
    esp_err_t ret = ESP_OK;
    cJSON *final_json = cJSON_CreateObject();
    cJSON *metadata = cJSON_CreateObject();
    cJSON *content = cJSON_CreateObject();

    char *ret_json_str = NULL;

    /* GOTO to delete_content_n_metadata if any of the JSON objects are not created */
    ESP_GOTO_ON_FALSE(final_json, ESP_ERR_NO_MEM, delete_all_json_objects, TAG, "Failed to create final JSON");
    ESP_GOTO_ON_FALSE(metadata, ESP_ERR_NO_MEM, delete_all_json_objects, TAG, "Failed to create metadata JSON");
    ESP_GOTO_ON_FALSE(content, ESP_ERR_NO_MEM, delete_all_json_objects, TAG, "Failed to create content JSON");

    cJSON_AddStringToObject(metadata, "role", "user");

    cJSON_AddStringToObject(final_json, "type", ESP_AGENT_MESSAGE_TYPE_AUDIO_STREAM_START);
    cJSON_AddStringToObject(final_json, "content_type", "json");
    cJSON_AddItemToObject(final_json, "metadata", metadata);
    cJSON_AddItemToObject(final_json, "content", content);

    ret_json_str = cJSON_PrintUnformatted(final_json);

    /* For keping the compiler happy about not using variable ret */
    /* Will be optimized away */
    if (ret) {}

    /* final_json has ownership of content and metadata*/
    goto only_delete_final_json;

delete_all_json_objects:
    if (metadata) {
        cJSON_Delete(metadata);
    }
    if (content) {
        cJSON_Delete(content);
    }
only_delete_final_json:
    if (final_json) {
        cJSON_Delete(final_json);
    }

    return ret_json_str;
}

char *esp_agent_messages_prepare_speech_conversation_end(esp_agent_handle_t handle)
{
    esp_err_t ret = ESP_OK;
    cJSON *final_json = cJSON_CreateObject();
    cJSON *metadata = cJSON_CreateObject();
    cJSON *content = cJSON_CreateObject();

    char *ret_json_str = NULL;

    /* GOTO to delete_content_n_metadata if any of the JSON objects are not created */
    ESP_GOTO_ON_FALSE(final_json, ESP_ERR_NO_MEM, delete_all_json_objects, TAG, "Failed to create final JSON");
    ESP_GOTO_ON_FALSE(metadata, ESP_ERR_NO_MEM, delete_all_json_objects, TAG, "Failed to create metadata JSON");
    ESP_GOTO_ON_FALSE(content, ESP_ERR_NO_MEM, delete_all_json_objects, TAG, "Failed to create content JSON");

    cJSON_AddStringToObject(metadata, "role", "user");

    cJSON_AddStringToObject(final_json, "type", ESP_AGENT_MESSAGE_TYPE_AUDIO_STREAM_END);
    cJSON_AddStringToObject(final_json, "content_type", "json");
    cJSON_AddItemToObject(final_json, "metadata", metadata);
    cJSON_AddItemToObject(final_json, "content", content);

    ret_json_str = cJSON_PrintUnformatted(final_json);

    /* For keping the compiler happy about not using variable ret */
    /* Will be optimized away */
    if (ret) {}

    /* final_json has ownership of content and metadata*/
    goto only_delete_final_json;

delete_all_json_objects:
    if (metadata) {
        cJSON_Delete(metadata);
    }
    if (content) {
        cJSON_Delete(content);
    }
only_delete_final_json:
    if (final_json) {
        cJSON_Delete(final_json);
    }

    return ret_json_str;
}

esp_err_t esp_agent_speech_conversation_start(esp_agent_handle_t handle)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = ESP_OK;

    esp_agent_t *agent = (esp_agent_t *)handle;
    if (agent->conversation_type != ESP_AGENT_CONVERSATION_SPEECH) {
        ESP_LOGE(TAG, "Conversation type is not speech");
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret = ESP_OK;
    char *speech_conversation_start_json_str = NULL;
    speech_conversation_start_json_str = esp_agent_messages_prepare_speech_conversation_start(agent);

    ESP_GOTO_ON_FALSE(speech_conversation_start_json_str, ESP_ERR_NO_MEM, end, TAG, "Failed to prepare speech conversation start");
    ESP_LOGD(TAG, "Speech conversation start: %s", speech_conversation_start_json_str);

    err = esp_agent_websocket_queue_message(agent, WS_SEND_MSG_TYPE_TEXT, speech_conversation_start_json_str, strlen(speech_conversation_start_json_str), pdMS_TO_TICKS(100));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to queue speech conversation start: %d", err);
    }

    if (ret) {}

end:
    if (speech_conversation_start_json_str) {
        free(speech_conversation_start_json_str);
    }
    return err;
}

esp_err_t esp_agent_speech_conversation_end(esp_agent_handle_t handle)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = ESP_OK;

    esp_agent_t *agent = (esp_agent_t *)handle;
    if (agent->conversation_type != ESP_AGENT_CONVERSATION_SPEECH) {
        ESP_LOGE(TAG, "Conversation type is not speech");
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret = ESP_OK;
    char *speech_conversation_end_json_str = NULL;
    speech_conversation_end_json_str = esp_agent_messages_prepare_speech_conversation_end(agent);

    ESP_GOTO_ON_FALSE(speech_conversation_end_json_str, ESP_ERR_NO_MEM, end, TAG, "Failed to prepare speech conversation end");
    ESP_LOGD(TAG, "Speech conversation end: %s", speech_conversation_end_json_str);

    err = esp_agent_websocket_queue_message(agent, WS_SEND_MSG_TYPE_TEXT, speech_conversation_end_json_str, strlen(speech_conversation_end_json_str), pdMS_TO_TICKS(100));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to queue speech conversation end: %d", err);
    }

    /* Just to avoid compiler warning */
    if (ret) {}

end:
    if (speech_conversation_end_json_str) {
        free(speech_conversation_end_json_str);
    }
    return err;
}

/* Send speech data */
esp_err_t esp_agent_send_speech(esp_agent_handle_t handle, const uint8_t *data, size_t len, TickType_t timeout)
{
    if (handle == NULL || data == NULL || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_agent_t *agent = (esp_agent_t *)handle;

    if (agent->conversation_type != ESP_AGENT_CONVERSATION_SPEECH) {
        ESP_LOGE(TAG, "Conversation type is not speech");
        return ESP_ERR_INVALID_STATE;
    }

    return esp_agent_websocket_queue_message(agent, WS_SEND_MSG_TYPE_BINARY, (const char *)data, len, timeout);
}

esp_err_t esp_agent_send_text(esp_agent_handle_t handle, const char *text, TickType_t timeout)
{
    if (handle == NULL || text == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_agent_t *agent = (esp_agent_t *)handle;
    if (agent->conversation_type != ESP_AGENT_CONVERSATION_TEXT) {
        ESP_LOGE(TAG, "Conversation type is not text");
        return ESP_ERR_INVALID_STATE;
    }

    char *text_json_str = esp_agent_messages_prepare_text(agent, text);
    if (text_json_str == NULL) {
        ESP_LOGE(TAG, "Failed to prepare text message");
        return ESP_FAIL;
    }

    esp_err_t err = esp_agent_websocket_queue_message(agent, WS_SEND_MSG_TYPE_TEXT, text_json_str, strlen(text_json_str), timeout);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to queue text data: %d", err);
    }

    if (text_json_str) {
        free(text_json_str);
    }
    return err;
}
