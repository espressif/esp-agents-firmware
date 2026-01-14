/**
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __AUDIO_PLAYBACK_H__
#define __AUDIO_PLAYBACK_H__

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "esp_err.h"
#include "esp_codec_dev.h"

typedef void* audio_playback_handle_t;

typedef struct {
    uint16_t frame_duration_ms;
    uint16_t sample_rate;
} audio_playback_audio_info_t;

typedef struct {
    audio_playback_audio_info_t audio_in_info;
    esp_codec_dev_sample_info_t out_codec_info;
    esp_codec_dev_handle_t out_dev_handle;
} audio_playback_config_t;

audio_playback_handle_t audio_playback_init(const audio_playback_config_t *config);

esp_err_t audio_playback_deinit(audio_playback_handle_t *handle);

esp_err_t audio_playback_start(audio_playback_handle_t *handle);

esp_err_t audio_playback_write(audio_playback_handle_t *handle, const uint8_t *data, size_t len);

esp_err_t audio_playback_remaining_bytes(audio_playback_handle_t *handle, size_t *remaining_bytes);

/**
 * @brief Play media data using ESP-GMF audio simple player
 *
 * Blocks until playback completion.
 *
 * @param handle The audio playback handle
 * @param media_url The media URL
 * @param data The media data (only applicable for embed:// schema)
 * @param len The length of the media data (only applicable for embed:// schema)
 * @return ESP_OK on success, otherwise an error code
 */
esp_err_t audio_playback_play_media_sync(audio_playback_handle_t *handle, const char *media_url, const uint8_t *data, size_t len);


/**
 * @brief Play media data using ESP-GMF audio simple player asynchronously
 *
 * Without blocking for playback completion.
 *
 * @param handle The audio playback handle
 * @param media_url The media URL
 * @param data The media data (only applicable for embed:// schema)
 * @param len The length of the media data (only applicable for embed:// schema)
 * @return ESP_OK on success, otherwise an error code
 */
esp_err_t audio_playback_play_media_async(audio_playback_handle_t *handle, const char *media_url, const uint8_t *data, size_t len);

#endif /* __AUDIO_PLAYBACK_H__ */
