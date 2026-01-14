/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

#include <esp_log.h>
#include <esp_event.h>
#include <esp_check.h>

#include <esp_agent.h>
#include <esp_agent_internal.h>
#include <esp_agent_events.h>

static const char *TAG = "esp_agent_events";

/* This should always be the last event handler in the chain. */
void esp_agent_internal_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_agent_message_data_t *data = (esp_agent_message_data_t *)event_data;

    switch (event_id) {
        case ESP_AGENT_EVENT_DATA_TYPE_TEXT:
            if (data->text.text) {
                ESP_LOGV(TAG, "Freeing text data buffer: %s", data->text.text);
                free((void *)data->text.text);
            }
            break;
        case ESP_AGENT_EVENT_DATA_TYPE_SPEECH:
            if (data->speech.data) {
                ESP_LOGV(TAG, "Freeing speech data buffer: %d bytes", data->speech.len);
                free((void *)data->speech.data);
            }
            break;
        case ESP_AGENT_EVENT_START:
            if (data->start.conversation_id) {
                ESP_LOGV(TAG, "Freeing conversation ID: %s", data->start.conversation_id);
                free((void *)data->start.conversation_id);
            }
            break;
        case ESP_AGENT_EVENT_DATA_TYPE_THINKING:
            if (data->thinking.thought) {
                ESP_LOGV(TAG, "Freeing thought data buffer: %s", data->thinking.thought);
                free((void *)data->thinking.thought);
            }
            break;
        default:
            break;
    }
}

esp_err_t esp_agent_post_event(esp_agent_handle_t handle, esp_agent_event_t event, esp_agent_message_data_t *data)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_agent_t *agent = (esp_agent_t *)handle;
    esp_err_t err = esp_event_post_to(agent->event_loop, AGENT_EVENT, event, data, sizeof(esp_agent_message_data_t), pdMS_TO_TICKS(1000));

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to post event: %x", err);
        return err;
    }
    return ESP_OK;
}

esp_err_t esp_agent_register_event_handler(esp_agent_handle_t handle, esp_agent_event_t event, esp_event_handler_t handler, void *user_data, esp_event_handler_instance_t *handler_instance)
{
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_agent_t *agent = (esp_agent_t *) handle;

    ESP_RETURN_ON_ERROR(esp_event_handler_instance_register_with(agent->event_loop, AGENT_EVENT, event, handler, user_data, handler_instance), TAG, "Failed to register internal event handler");

    /**
     * ESP Event Loop uses linked list to interally track the event handlers in any event queue.
     * We need to free the data allocated for the event(such as user/assistant transcript)
     * Hence, we de-register and re-register the internal handler so that it is always executed at the last
     */
    if(agent->internal_event_handler){
        esp_agent_unregister_event_handler(handle, agent->internal_event_handler, ESP_EVENT_ANY_ID);
    }
    esp_err_t err = ESP_OK;
    err = esp_event_handler_instance_register_with(agent->event_loop, AGENT_EVENT, ESP_EVENT_ANY_ID, esp_agent_internal_event_handler, NULL, &agent->internal_event_handler);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to (re) register the internal event handler");
    }

    return ESP_OK;
}


esp_err_t esp_agent_unregister_event_handler(esp_agent_handle_t handle, esp_event_handler_instance_t *handler_instance, esp_agent_event_t event)
{
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_agent_t *agent = (esp_agent_t *) handle;

    esp_err_t err = esp_event_handler_instance_unregister_with(agent->event_loop, AGENT_EVENT, event, handler_instance);
    return err;
}
