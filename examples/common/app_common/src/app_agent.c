/**
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <esp_log.h>
#include <esp_check.h>

#include <esp_agent.h>

#include <agent_setup.h>
#include <setup/rainmaker.h>
#include <board_defs.h>
#include <esp_console.h>

#include "app_audio.h"
#include "app_agent.h"
#include "app_device.h"

static const char *TAG = "app_agent";

typedef struct {
    bool initialized;
    app_agent_state_t state;
    esp_agent_handle_t agent_handle;
    esp_event_handler_instance_t agent_event_handler;
    esp_event_handler_instance_t agent_setup_event_handler;
    app_agent_config_t config;
} app_agent_data_t;

app_agent_data_t g_app_agent_data;

static inline void app_agent_update_state(app_agent_state_t state)
{
    g_app_agent_data.state = state;
    app_device_event_enqueue(DEVICE_EVENT_AGENT_STATE_CHANGED);
}

void app_agent_default_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    esp_agent_message_data_t *data = (esp_agent_message_data_t *) event_data;

    switch (event_id) {
        case ESP_AGENT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "Agent Connected. Waiting to start conversation.");
            app_agent_update_state(APP_AGENT_STATE_CONNECTED);
            break;
        case ESP_AGENT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "Agent Not Connected");
            app_agent_update_state(APP_AGENT_STATE_DISCONNECTED);
            // Stop microphone to prevent sending data while disconnected
            app_device_event_enqueue(DEVICE_EVENT_SLEEP);
            break;
        case ESP_AGENT_EVENT_SPEECH_START:
            ESP_LOGD(TAG, "ESP Agent Received Speech Start");
            app_device_event_enqueue(DEVICE_EVENT_SPEECH_START);
            break;
        case ESP_AGENT_EVENT_SPEECH_END:
            ESP_LOGD(TAG, "ESP Agent Received Speech End");
            app_device_event_enqueue(DEVICE_EVENT_SPEECH_END);
            break;
        case ESP_AGENT_EVENT_DATA_TYPE_TEXT:
            {
                if (data->text.generation_stage == ESP_AGENT_MESSAGE_GENERATION_STAGE_FINAL) {
                    break;
                }

                app_device_event_t event = DEVICE_EVENT_SET_USER_TEXT;
                char *text = NULL;
                if (data->text.text) {
                    text = strdup(data->text.text);
                }

                if (data->text.role == ESP_AGENT_MESSAGE_ROLE_USER) {
                    event = DEVICE_EVENT_SET_USER_TEXT;
                } else if (data->text.role == ESP_AGENT_MESSAGE_ROLE_ASSISTANT) {
                    event = DEVICE_EVENT_SET_ASSISTANT_TEXT;
                }

                device_event_data_t event_data = { .text = text };
                app_device_event_enqueue_with_data(event, &event_data);
            }
            break;
        case ESP_AGENT_EVENT_DATA_TYPE_SPEECH:
            ESP_LOGD(TAG, "ESP Agent Speech data: %d", data->speech.len);
            app_audio_play_speech((uint8_t *)data->speech.data, data->speech.len);
            break;
        case ESP_AGENT_EVENT_DATA_TYPE_THINKING:
            /* Display the thought in gray color */
            printf("\033[90mThought: %s\033[0m\n", data->thinking.thought);
            break;
        case ESP_AGENT_EVENT_ERROR:
            if (data->error.error == ESP_AGENT_AUDIO_CONVERSATION_ERROR) {
                ESP_LOGE(TAG, "ESP Agent Audio Conversation Error");
                /* Device state will be changed to sleep on ESP_AGENT_EVENT_DISCONNECT */
                esp_agent_stop(g_app_agent_data.agent_handle);
            }

            break;
        case ESP_AGENT_EVENT_START:
            app_agent_update_state(APP_AGENT_STATE_STARTED);
            ESP_LOGI(TAG, "ESP Agent Started");
            break;
        default:
            break;
    }
}

esp_err_t app_agent_send_speech(uint8_t *audio_data, size_t audio_data_len)
{
    if (g_app_agent_data.state != APP_AGENT_STATE_STARTED) {
        return ESP_ERR_INVALID_STATE;
    }
    return esp_agent_send_speech(g_app_agent_data.agent_handle, audio_data, audio_data_len, pdMS_TO_TICKS(1000));
}

void app_agent_start_task(void *arg)
{
    char *agent_id = agent_setup_get_agent_id();
    char *refresh_token = agent_setup_get_refresh_token();
    if (!agent_id || !refresh_token) {
        ESP_LOGE(TAG, "Agent ID or refresh token not found");
        return;
    }

    esp_err_t ret = ESP_OK;

    ESP_GOTO_ON_ERROR(esp_agent_set_agent_id(g_app_agent_data.agent_handle, agent_id), end, TAG, "Failed to set agent ID");
    ESP_GOTO_ON_ERROR(esp_agent_set_refresh_token(g_app_agent_data.agent_handle, refresh_token), end, TAG, "Failed to set refresh token");
    ESP_GOTO_ON_ERROR(app_audio_start(), end, TAG, "Failed to start audio pipeline");
    ESP_GOTO_ON_ERROR(app_agent_connect(), end, TAG, "Failed to start agent");

    app_device_event_enqueue(DEVICE_EVENT_SYSTEM_INITIALIZED);

end:
    (void)ret; /* Suppress unused variable warning */
    vTaskDelete(NULL);
}

static void agent_setup_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    switch (event_id) {
        case AGENT_SETUP_EVENT_AGENT_ID_UPDATE:
            {
                ESP_LOGI(TAG, "Agent ID updated");
                if (g_app_agent_data.agent_handle) {
                    char *agent_id = agent_setup_get_agent_id();
                    ESP_RETURN_VOID_ON_ERROR(esp_agent_set_agent_id(g_app_agent_data.agent_handle, agent_id), TAG, "Failed to set agent ID");
                }
            }
            break;

        case AGENT_SETUP_EVENT_START:
            {
                ESP_LOGI(TAG, "Agent setup completed");
                xTaskCreate(app_agent_start_task, "app_agent_start_task", 4096, NULL, 5, NULL);
            }
            break;

        default:
            break;
    }
}

esp_err_t app_agent_speech_conversation_start(void)
{
    if (g_app_agent_data.state != APP_AGENT_STATE_STARTED) {
        return ESP_ERR_INVALID_STATE;
    }
    return esp_agent_speech_conversation_start(g_app_agent_data.agent_handle);
}

esp_err_t app_agent_speech_conversation_end(void)
{
    if (g_app_agent_data.state != APP_AGENT_STATE_STARTED) {
        return ESP_ERR_INVALID_STATE;
    }
    return esp_agent_speech_conversation_end(g_app_agent_data.agent_handle);
}

esp_err_t app_agent_connect(void)
{
    if (!g_app_agent_data.agent_handle) {
        ESP_LOGE(TAG, "Can't start agent, handle not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    app_agent_update_state(APP_AGENT_STATE_CONNECTING);
    esp_err_t ret = esp_agent_start(g_app_agent_data.agent_handle, NULL);
    if (ret != ESP_OK) {
        app_agent_update_state(APP_AGENT_STATE_DISCONNECTED);
    }
    return ret;
}

esp_err_t app_agent_init(app_agent_config_t *config)
{
    if (g_app_agent_data.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (!config->event_handler) {
        ESP_LOGE(TAG, "Event handler is required");
        return ESP_ERR_INVALID_ARG;
    }

    /* Initialize the agent setup: agent ID, refresh token, etc. */
    ESP_RETURN_ON_ERROR(agent_setup_init(), TAG, "Failed to initialize agent setup");

    ESP_RETURN_ON_ERROR(esp_event_handler_register(AGENT_SETUP_EVENT, ESP_EVENT_ANY_ID, agent_setup_event_handler, NULL), TAG, "Failed to register agent event handler");

    /* Initialize esp_agent without agent_id and refresh_token */
    esp_agent_audio_config_t upload_audio_config = {
        .format = ESP_AGENT_CONVERSATION_AUDIO_FORMAT_OPUS,
        .sample_rate = CONFIG_AUDIO_UPLOAD_SAMPLE_RATE,
        .frame_duration = CONFIG_AUDIO_UPLOAD_FRAME_DURATION_MS,
    };
    esp_agent_audio_config_t download_audio_config = {
        .format = ESP_AGENT_CONVERSATION_AUDIO_FORMAT_OPUS,
        .sample_rate = CONFIG_AUDIO_DOWNLOAD_SAMPLE_RATE,
        .frame_duration = CONFIG_AUDIO_DOWNLOAD_FRAME_DURATION_MS,
    };

    esp_agent_config_t agent_config = {
        .conversation_type = ESP_AGENT_CONVERSATION_SPEECH,
        .upload_audio_config = &upload_audio_config,
        .download_audio_config = &download_audio_config,
    };

    g_app_agent_data.agent_handle = esp_agent_init(&agent_config);
    if (g_app_agent_data.agent_handle == NULL) {
        ESP_LOGE(TAG, "Failed to initialize agent");
        return ESP_FAIL;
    }

    /* Register event handler */
    esp_event_handler_t handler = config->event_handler;
    ESP_RETURN_ON_ERROR(esp_agent_register_event_handler(g_app_agent_data.agent_handle, ESP_EVENT_ANY_ID, handler, NULL, &g_app_agent_data.agent_event_handler), TAG, "Failed to register agent event handler");

    g_app_agent_data.state = APP_AGENT_STATE_DISCONNECTED;
    g_app_agent_data.initialized = true;
    g_app_agent_data.config = *config;

    return ESP_OK;
}

esp_err_t app_agent_start(void)
{
    esp_err_t ret = ESP_OK;
    /* Initialize ESP RainMaker.
    * This device manual shows up in the ESP RainMaker Home app after the device has been set up. This should be set
    * in the board_defs.h file.
    */
    ESP_RETURN_ON_ERROR(setup_rainmaker_init(BOARD_DEVICE_MANUAL_URL), TAG, "Failed to initialize RainMaker setup");

    /* Start network provisioning. This will start the network and connect to the cloud.
    * If the device has not been set up yet, it will start the setup process.
    * RainMaker will start automatically when network connectivity is established.
    */
    ESP_RETURN_ON_ERROR(agent_setup_start(), TAG, "Failed to start network provisioning");
    return ret;
}

bool app_agent_is_active(void)
{
    return (g_app_agent_data.state == APP_AGENT_STATE_STARTED);
}

app_agent_state_t app_agent_get_state(void)
{
    return g_app_agent_data.state;
}

esp_err_t app_agent_register_tool(const char *name, esp_agent_tool_handler_t tool_handler, void *user_data)
{
    if (!g_app_agent_data.agent_handle) {
        ESP_LOGE(TAG, "Agent handle not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t err = esp_agent_register_local_tool(g_app_agent_data.agent_handle, name, tool_handler, user_data);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to register local tool: %s", name);
    }
    return err;
}

esp_err_t app_agent_tool_unregister(const char *name)
{
    if (!g_app_agent_data.agent_handle) {
        ESP_LOGE(TAG, "Agent handle not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    return esp_agent_unregister_local_tool(g_app_agent_data.agent_handle, name);
}
