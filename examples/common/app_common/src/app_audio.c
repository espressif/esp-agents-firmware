/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <esp_check.h>
#include <esp_log.h>
#include <driver/i2s_std.h>
#include <nvs_flash.h>
#include <agent_setup.h>
#include <agent_console.h>
#include <setup/rainmaker.h>
#include <esp_agent.h>

#include <esp_board_device.h>
#include <dev_audio_codec.h>

#include "app_audio.h"
#include "app_agent.h"
#include "app_device.h"

static const char *TAG = "app_audio";

typedef struct {
    bool initialized;
    audio_recorder_handle_t recorder_handle;
    audio_playback_handle_t playback_handle;
    esp_codec_dev_handle_t speaker_handle;
    app_audio_microphone_state_t microphone_state;
    bool speaker_active;
    bool audio_download_complete;
    bool audio_playback_complete;
    EventGroupHandle_t event_group;
    uint8_t volume;
} app_audio_data_t;

typedef struct {
    const char *dac;
    esp_codec_dev_vol_map_t vol_map[2];
} dac_volume_curve_map;

app_audio_data_t g_app_audio_data;

#define APP_AUDIO_NVS_NAMESPACE "app_audio"
#define APP_AUDIO_NVS_KEY_VOLUME "volume"

#define AUDIO_SEND_BUFFER_SIZE 1024
#define OPUS_DUMMY_FRAME_DATA_SIZE 320 // random size for dummy audio data (all 0s)

#define AUDIO_DOWNLOAD_COMPLETE_BIT (1 << 2)

static const esp_codec_dev_sample_info_t g_audio_cfg = {
    .sample_rate = 16000,
    .channel = 2,
    .bits_per_sample = 32,
};

static dac_volume_curve_map g_dac_volume_maps[] = {
    {
        .dac = "ES8311",
        .vol_map = {
            {
                .vol = 0,
                .db_value = -20,
            },
            {
                .vol = 100,
                .db_value = 8,
            }
        }
    },
    {
        .dac = "AW88298",
        .vol_map = {
            {
                .vol = 0,
                .db_value = -30,
            },
            {
                .vol = 100,
                .db_value = 0,
            }
        }
    }
};

static esp_codec_dev_vol_curve_t g_speaker_volume_curve = {
    .count = 2,
};

static esp_err_t nvs_get_volume(uint8_t *volume)
{
    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open(APP_AUDIO_NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (ret != ESP_OK) {
        return ret;
    }

    ret = nvs_get_u8(nvs_handle, APP_AUDIO_NVS_KEY_VOLUME, volume);
    if (ret != ESP_OK) {
        goto end;
    }

end:
    nvs_close(nvs_handle);
    return ret;
}

static esp_err_t nvs_set_volume(uint8_t volume)
{
    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open(APP_AUDIO_NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (ret != ESP_OK) {
        return ret;
    }

    ret = nvs_set_u8(nvs_handle, APP_AUDIO_NVS_KEY_VOLUME, volume);
    if (ret != ESP_OK) {
        goto end;
    }

end:
    nvs_close(nvs_handle);
    return ret;
}

static esp_err_t app_audio_set_volume_handler(int argc, char **argv)
{
    if (argc != 2) {
        ESP_LOGE(TAG, "Usage: set-volume <volume>");
        return ESP_ERR_INVALID_ARG;
    }
    const char *volume = argv[1];
    return app_audio_set_playback_volume(atoi(volume));
}

static esp_err_t register_audio_commands()
{
    esp_console_cmd_t cmd = {
        .command = "set-volume",
        .help = "Set the volume of the playback device\nUsage: set-volume <volume>",
        .func = app_audio_set_volume_handler,
    };

    return agent_console_register_command(&cmd);
}

static void audio_recorder_event_handler(audio_recorder_handle_t handle, audio_recorder_event_t event, void *user_data)
{
    switch (event) {
        case AUDIO_RECORDER_EVENT_WAKEUP_START:
            app_device_event_enqueue(DEVICE_EVENT_WAKEUP);
            break;
        case AUDIO_RECORDER_EVENT_WAKEUP_END:
            app_device_event_enqueue(DEVICE_EVENT_SLEEP);
            break;
        default:
            break;
    }
}

static void audio_microphone_task(void *arg)
{
    uint8_t *audio_data = (uint8_t *) malloc(AUDIO_SEND_BUFFER_SIZE);
    assert(audio_data);
    size_t audio_data_len = 0;
    uint8_t *dummy_audio_data = (uint8_t *) calloc(OPUS_DUMMY_FRAME_DATA_SIZE, 1);

    ESP_LOGI(TAG, "Audio microphone task started");
    while (true) {
        audio_recorder_read(g_app_audio_data.recorder_handle, audio_data, AUDIO_SEND_BUFFER_SIZE, &audio_data_len);

        esp_err_t err = ESP_OK;
        switch (g_app_audio_data.microphone_state) {
            case MICROPHONE_STATE_START:
                err = app_agent_send_speech(audio_data, audio_data_len);
                break;
            case MICROPHONE_STATE_PAUSE:
                err = app_agent_send_speech(dummy_audio_data, OPUS_DUMMY_FRAME_DATA_SIZE);
                break;
            case MICROPHONE_STATE_STOP:
                vTaskDelay(pdMS_TO_TICKS(10));
                continue; // While loop
            default:
                break;
        }

        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Failed to send speech data: %s", esp_err_to_name(err));
            app_device_event_enqueue(DEVICE_EVENT_SLEEP);
        }
    }

    free(audio_data);
    free(dummy_audio_data);
    vTaskDelete(NULL);
}

static esp_err_t audio_init_micrphone()
{
    dev_audio_codec_handles_t *codec_handles = NULL;
    ESP_RETURN_ON_ERROR(esp_board_device_get_handle("audio_adc", (void **)&codec_handles), TAG, "Failed to get audio_adc handle");
    esp_codec_dev_handle_t microphone_handle = codec_handles->codec_dev;

    ESP_RETURN_ON_ERROR(esp_codec_dev_open(microphone_handle, (esp_codec_dev_sample_info_t *)&g_audio_cfg), TAG, "Failed to open microphone");
    ESP_RETURN_ON_ERROR(esp_codec_dev_set_in_gain(microphone_handle, 30.0f), TAG, "Failed to set microphone gain");

    audio_recorder_config_t config = {
        .format = "RMNM",
        .in_dev_handle = microphone_handle,
        .sample_rate = CONFIG_AUDIO_UPLOAD_SAMPLE_RATE,
        .frame_duration_ms = CONFIG_AUDIO_UPLOAD_FRAME_DURATION_MS,
    };

    g_app_audio_data.recorder_handle = audio_recorder_init(&config);
    if (g_app_audio_data.recorder_handle == NULL) {
        return ESP_FAIL;
    }

    audio_recorder_add_event_cb(g_app_audio_data.recorder_handle, audio_recorder_event_handler, NULL);

    return ESP_OK;
}

static esp_err_t audio_calibrate_volume_curve(esp_codec_dev_handle_t speaker_handle, const char *dac_name)
{
    /**
     * Set the volume curve for the speaker
     * The default volume curve is -96dB to 0dB for volume 0 to 100
     * However some DACs(like ES8311) support gain up to 32dB.
     * The audio volume on some board can be too low, even at volume 100 using default curve.
     * So we set a custom volume curve to increase the volume.
     */

    for (int i = 0; i < sizeof(g_dac_volume_maps) / sizeof(g_dac_volume_maps[0]); i++) {
        if (strncasecmp(g_dac_volume_maps[i].dac, dac_name, strlen(g_dac_volume_maps[i].dac)) == 0) {
            g_speaker_volume_curve.vol_map = g_dac_volume_maps[i].vol_map;
            return esp_codec_dev_set_vol_curve(speaker_handle, &g_speaker_volume_curve);
        }
    }
    ESP_LOGW(TAG, "Unknown DAC: %s, using default volume curve", dac_name);
    return ESP_FAIL;
}

static esp_err_t audio_init_speaker()
{
    dev_audio_codec_handles_t *codec_handles = NULL;
    dev_audio_codec_config_t *codec_config = NULL;
    ESP_RETURN_ON_ERROR(esp_board_device_get_handle("audio_dac", (void **)&codec_handles), TAG, "Failed to get audio_dac handle");
    ESP_RETURN_ON_ERROR(esp_board_device_get_config("audio_dac", (void **)&codec_config), TAG, "Failed to get audio_dac config");
    esp_codec_dev_handle_t speaker_handle = codec_handles->codec_dev;
    g_app_audio_data.speaker_handle = speaker_handle;

    esp_codec_dev_sample_info_t fs = {
        .sample_rate = 16000,
        .channel = 2,
        .bits_per_sample = 32,
    };

    ESP_RETURN_ON_ERROR(esp_codec_dev_open(speaker_handle, &fs), TAG, "Failed to open speaker");
    uint8_t volume = 0;
    esp_err_t err = nvs_get_volume(&volume);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to get volume from NVS, using default value: %d", CONFIG_APP_AUDIO_DEFAULT_PLAYBACK_VOLUME);
        volume = CONFIG_APP_AUDIO_DEFAULT_PLAYBACK_VOLUME;
    }

    /* This should be called before setting the boot-up volume */
    audio_calibrate_volume_curve(speaker_handle, codec_config->chip);

    ESP_RETURN_ON_ERROR(esp_codec_dev_set_out_vol(speaker_handle, volume), TAG, "Failed to set speaker volume");
    g_app_audio_data.volume = volume;

    audio_playback_config_t config = {
        .audio_in_info = {
            .sample_rate = CONFIG_AUDIO_DOWNLOAD_SAMPLE_RATE,
            .frame_duration_ms = CONFIG_AUDIO_DOWNLOAD_FRAME_DURATION_MS,
        },
        .out_codec_info = g_audio_cfg,
        .out_dev_handle = speaker_handle,
    };

    g_app_audio_data.playback_handle = audio_playback_init(&config);
    if (g_app_audio_data.playback_handle == NULL) {
        return ESP_FAIL;
    }

    return ESP_OK;
}

void download_complete_task(void *arg)
{
    while (true) {
        /* Wait for the download complete bit to be set */
        /* This will be set by the `app_audio_speaker_download_complete` function */
        xEventGroupWaitBits(g_app_audio_data.event_group, AUDIO_DOWNLOAD_COMPLETE_BIT, pdTRUE, pdFALSE, portMAX_DELAY);

        size_t remaining_bytes = 0;
        do {
            audio_playback_remaining_bytes(g_app_audio_data.playback_handle, &remaining_bytes);
            vTaskDelay(pdMS_TO_TICKS(10));
        } while (remaining_bytes > 0);

        vTaskDelay(pdMS_TO_TICKS(100));

        ESP_LOGI(TAG, "Speaker playback complete");
        g_app_audio_data.audio_playback_complete = true;
        app_device_event_enqueue(DEVICE_EVENT_SPEECH_PLAYBACK_COMPLETE);
    }

    vTaskDelete(NULL);
}

static esp_err_t app_audio_get_volume_cb(uint8_t *volume)
{
    if (!g_app_audio_data.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    *volume = g_app_audio_data.volume;
    return ESP_OK;
}

static esp_err_t app_audio_set_volume_cb(uint8_t volume)
{
    return app_audio_set_playback_volume(volume);
}


esp_err_t app_audio_init(void)
{
    if (g_app_audio_data.initialized) {
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(audio_init_micrphone(), TAG, "Failed to initialize microphone");
    ESP_RETURN_ON_ERROR(audio_init_speaker(), TAG, "Failed to initialize speaker");
    ESP_RETURN_ON_ERROR(register_audio_commands(), TAG, "Failed to register audio commands");

    /* Register volume callbacks with RainMaker */
    ESP_RETURN_ON_ERROR(setup_rainmaker_register_volume_callbacks(app_audio_get_volume_cb, app_audio_set_volume_cb), TAG, "Failed to register volume callbacks");

    g_app_audio_data.event_group = xEventGroupCreate();
    g_app_audio_data.initialized = true;

    return ESP_OK;
}

esp_err_t app_audio_start(void)
{
    if (!g_app_audio_data.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    xTaskCreate(audio_microphone_task, "audio_microphone_task", 1024 * 4, NULL, 8, NULL);
    xTaskCreate(download_complete_task, "download_complete_task", 1024 * 4, NULL, 8, NULL);

    ESP_RETURN_ON_ERROR(audio_recorder_start(g_app_audio_data.recorder_handle), TAG, "Failed to start audio recorder");
    ESP_RETURN_ON_ERROR(audio_playback_start(g_app_audio_data.playback_handle), TAG, "Failed to start audio playback");

    return ESP_OK;
}

esp_err_t app_audio_play_speech(uint8_t *data, size_t data_len)
{
    if (!g_app_audio_data.speaker_active) {
        ESP_LOGD(TAG, "Speaker is not active. Skipping playback.");
        return ESP_OK;
    }

    if (g_app_audio_data.audio_playback_complete) {
        ESP_LOGD(TAG, "Playback complete. Dropping speech data: %d bytes", data_len);
        return ESP_OK;
    }

    return audio_playback_write(g_app_audio_data.playback_handle, data, data_len);
}


esp_err_t app_audio_set_playback_volume(uint8_t volume)
{
    if (!g_app_audio_data.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (volume > 100) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_RETURN_ON_ERROR(nvs_set_volume(volume), TAG, "Failed to set volume in NVS");
    ESP_RETURN_ON_ERROR(esp_codec_dev_set_out_vol(g_app_audio_data.speaker_handle, volume), TAG, "Failed to set playback volume");
    ESP_RETURN_ON_ERROR(setup_rainmaker_update_volume(volume), TAG, "Failed to update volume in RainMaker");
    g_app_audio_data.volume = volume;
    ESP_LOGI(TAG, "Volume set to %d", volume);
    return ESP_OK;
}

esp_err_t app_audio_microphone_set_state(app_audio_microphone_state_t state)
{
    switch (state) {
        case MICROPHONE_STATE_START:
            ESP_LOGI(TAG, "Starting microphone");
            break;
        case MICROPHONE_STATE_PAUSE:
            ESP_LOGI(TAG, "Pausing microphone");
            break;
        case MICROPHONE_STATE_STOP:
            ESP_LOGI(TAG, "Stopping microphone");
            break;
        default:
            break;
    }
    g_app_audio_data.microphone_state = state;

    return ESP_OK;
}


esp_err_t app_audio_speaker_start(void)
{
    ESP_LOGI(TAG, "Starting speaker");
    g_app_audio_data.speaker_active = true;
    g_app_audio_data.audio_playback_complete = false;

    return ESP_OK;
}

esp_err_t app_audio_speaker_stop(void)
{
    ESP_LOGI(TAG, "Stopping speaker");
    g_app_audio_data.speaker_active = false;
    return ESP_OK;
}

esp_err_t app_audio_speaker_download_complete(void)
{
    ESP_LOGI(TAG, "Speaker download complete");

    xEventGroupSetBits(g_app_audio_data.event_group, AUDIO_DOWNLOAD_COMPLETE_BIT);

    return ESP_OK;
}

esp_err_t app_audio_set_awake(bool awake)
{
    return audio_recorder_stay_awake(g_app_audio_data.recorder_handle, awake);
}

esp_err_t app_audio_trigger_sleep(void)
{
    /* AFE doesn't emit wakeup_end event when manually triggered */
    return audio_recorder_trigger_sleep(g_app_audio_data.recorder_handle);
}

esp_err_t app_audio_play_media_sync(const char *media_url, const uint8_t *data, size_t data_len)
{
    return audio_playback_play_media_sync(g_app_audio_data.playback_handle, media_url, data, data_len);
}

esp_err_t app_audio_play_media_async(const char *media_url, const uint8_t *data, size_t data_len)
{
    return audio_playback_play_media_async(g_app_audio_data.playback_handle, media_url, data, data_len);
}
