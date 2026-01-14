/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <esp_agent.h>

#include <audio_recorder.h>
#include <audio_playback.h>

typedef enum {
    MICROPHONE_STATE_START,
    MICROPHONE_STATE_PAUSE,
    MICROPHONE_STATE_STOP,
    MICROPHONE_STATE_MAX,
} app_audio_microphone_state_t;


/**
 * @brief Initialize the audio pipeline
 *
 * This function initializes the audio pipeline.
 * It initializes the audio recorder and starts the audio pipeline.
 *
 * @return ESP_OK on success, otherwise an error code
 */
esp_err_t app_audio_init(void);

esp_err_t app_audio_start(void);

/**
 * @brief Set the volume of the playback device
 *
 * This function sets the volume of the playback device,
 * and stores it in NVS for furthur use.
 *
 * @param volume The volume to set
 * @return ESP_OK on success, otherwise an error code
 */
esp_err_t app_audio_set_playback_volume(uint8_t volume);

esp_err_t app_audio_play_speech(uint8_t *data, size_t data_len);

esp_err_t app_audio_microphone_set_state(app_audio_microphone_state_t state);

esp_err_t app_audio_speaker_start(void);

esp_err_t app_audio_speaker_stop(void);

esp_err_t app_audio_speaker_download_complete(void);

esp_err_t app_audio_play_media_sync(const char *media_url, const uint8_t *data, size_t data_len);

esp_err_t app_audio_play_media_async(const char *media_url, const uint8_t *data, size_t data_len);

esp_err_t app_audio_trigger_sleep(void);

esp_err_t app_audio_set_awake(bool awake);
