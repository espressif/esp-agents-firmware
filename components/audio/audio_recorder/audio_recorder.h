/**
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __AUDIO_RECORDER_H__
#define __AUDIO_RECORDER_H__

#include <stdbool.h>
#include <esp_err.h>

#include <esp_codec_dev.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void *audio_recorder_handle_t;

/**
 * @brief Audio recorder configuration
 *
 * @param format Channel configuration for AFE
 * @param in_dev_handle Handle to the input device
 * @param sample_rate Sample rate in Hz
 * @param frame_duration_ms Frame duration in milliseconds(for OPUS encoding only)
 *
 * @note: All of the channels should be 16-bit, and the sample rate should be 16000.
 */
typedef struct {
    const char *format;
    esp_codec_dev_handle_t in_dev_handle;
    uint16_t sample_rate;
    uint8_t frame_duration_ms;
} audio_recorder_config_t;

typedef enum {
    AUDIO_RECORDER_EVENT_WAKEUP_START,
    AUDIO_RECORDER_EVENT_WAKEUP_END,
    AUDIO_RECORDER_EVENT_VAD_START,
    AUDIO_RECORDER_EVENT_VAD_END,
    AUDIO_RECORDER_EVENT_MAX,
} audio_recorder_event_t;

typedef void (*audio_recorder_event_cb_t)(audio_recorder_handle_t handle, audio_recorder_event_t event, void *user_data);

/* @brief Initialize the audio recorder
 *
 * This function initializes the audio recorder with the given configuration.
 * However, the audio processing doesn't start until audio_recorder_start() is called.
 *
 * @param config The configuration for the audio recorder
 * @return The handle to the audio recorder
 */
audio_recorder_handle_t audio_recorder_init(const audio_recorder_config_t *config);

/* @brief Start the audio recorder
 *
 * This function starts the audio recorder.
 * It starts the audio processing and reads the audio data from the codec device.
 *
 * @param handle The handle to the audio recorder
 * @return ESP_OK on success, otherwise an error code
 */
esp_err_t audio_recorder_start(audio_recorder_handle_t handle);

esp_err_t audio_recorder_read(audio_recorder_handle_t handle, uint8_t *data, size_t len, size_t *read_len);

esp_err_t audio_recorder_deinit(audio_recorder_handle_t handle);

esp_err_t audio_recorder_add_event_cb(audio_recorder_handle_t handle, audio_recorder_event_cb_t cb, void *user_data);

esp_err_t audio_recorder_stay_awake(audio_recorder_handle_t handle, bool awake);

esp_err_t audio_recorder_trigger_sleep(audio_recorder_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif /* __AUDIO_RECORDER_H__ */
