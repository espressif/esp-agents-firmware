/**
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <freertos/FreeRTOS.h>

#include <string.h>
#include <esp_log.h>

#include <esp_gmf_pool.h>
#include <esp_gmf_pipeline.h>
#include <esp_gmf_data_bus.h>
#include <esp_gmf_new_databus.h>
#include <esp_gmf_io_embed_flash.h>
#include <esp_audio_simple_player.h>
#include <esp_audio_simple_player_advance.h>

#include <esp_gmf_rate_cvt.h>
#include <esp_gmf_bit_cvt.h>
#include <esp_gmf_ch_cvt.h>
#include <esp_gmf_audio_dec.h>
#include <esp_opus_dec.h>

#include "audio_common.h"
#include "audio_playback.h"

static const char *TAG = "audio_playback";

#define AUDIO_PLAYBACK_FIFO_BLOCK_COUNT 5
#define AUDIO_PLAYBACK_FIFO_BLOCK_SIZE 512  // Block size for OPUS data

typedef struct audio_playback_s {
    esp_gmf_pipeline_handle_t pipeline_handle;
    esp_gmf_task_handle_t task_handle;
    esp_codec_dev_handle_t out_dev_handle;
    esp_gmf_db_handle_t fifo;
    audio_playback_audio_info_t audio_in_info;
    esp_codec_dev_sample_info_t out_codec_info;
    const uint8_t *asp_embed_data;
    size_t asp_embed_data_len;
    esp_asp_handle_t asp_handle;
    bool started;
} audio_playback_t;

static esp_gmf_err_io_t playback_inport_acquire_read(void *handle, esp_gmf_data_bus_block_t *blk, int wanted_size, int block_ticks)
{
    esp_gmf_data_bus_block_t _blk = {0};
    audio_playback_t *playback = (audio_playback_t *)handle;

    esp_gmf_err_io_t err = esp_gmf_db_acquire_read(playback->fifo, &_blk, wanted_size, block_ticks);
    if (err != ESP_GMF_IO_OK) {
        blk->valid_size = 0;
        return ESP_GMF_IO_OK;
    }

    memcpy(blk->buf, _blk.buf, _blk.valid_size);
    blk->valid_size = _blk.valid_size;
    blk->is_last = _blk.is_last;

    err = esp_gmf_db_release_read(playback->fifo, &_blk, block_ticks);
    if (err != ESP_GMF_IO_OK) {
        ESP_LOGW(TAG, "Failed to release ESP-GMF data bus read: %x", err);
    }

    return ESP_GMF_IO_OK;
}

static esp_gmf_err_io_t playback_inport_release_read(void *handle, esp_gmf_data_bus_block_t *blk, int block_ticks)
{
    return ESP_GMF_IO_OK;
}

static esp_gmf_err_io_t playback_outport_acquire_write(void *handle, esp_gmf_data_bus_block_t *blk, int wanted_size, int block_ticks)
{
    return ESP_GMF_IO_OK;
}

static esp_gmf_err_io_t playback_outport_release_write(void *handle, esp_gmf_data_bus_block_t *blk, int block_ticks)
{
    audio_playback_t *playback = (audio_playback_t *)handle;

    ESP_LOGD(TAG, "Writing audio data to codec device: %d", blk->valid_size);
    esp_codec_dev_write(playback->out_dev_handle, blk->buf, blk->valid_size);

    return ESP_GMF_IO_OK;
}

static esp_gmf_err_t pipeline_setup_elements(esp_gmf_pipeline_handle_t pipeline_handle, audio_playback_t *playback)
{
    esp_gmf_err_t err = ESP_GMF_ERR_OK;
    esp_gmf_element_handle_t ele = NULL;

    err = esp_gmf_pipeline_get_el_by_name(pipeline_handle, "aud_rate_cvt", &ele);
    if (err != ESP_GMF_ERR_OK) {
        ESP_LOGE(TAG, "Failed to get rate cvt element: %x", err);
    } else {
        esp_gmf_rate_cvt_set_dest_rate(ele, playback->out_codec_info.sample_rate);
    }

    err = esp_gmf_pipeline_get_el_by_name(pipeline_handle, "aud_bit_cvt", &ele);
    if (err != ESP_GMF_ERR_OK) {
        ESP_LOGE(TAG, "Failed to get bit cvt element: %x", err);
    } else {
        esp_gmf_bit_cvt_set_dest_bits(ele, playback->out_codec_info.bits_per_sample);
    }

    err = esp_gmf_pipeline_get_el_by_name(pipeline_handle, "aud_ch_cvt", &ele);
    if (err != ESP_GMF_ERR_OK) {
        ESP_LOGE(TAG, "Failed to get ch cvt element: %x", err);
    } else {
        esp_gmf_ch_cvt_set_dest_channel(ele, playback->out_codec_info.channel);
    }

    // Get the OPUS decoder element - it will automatically decode OPUS format
    err = esp_gmf_pipeline_get_el_by_name(pipeline_handle, "aud_dec", &ele);
    if (err != ESP_GMF_ERR_OK) {
        ESP_LOGE(TAG, "Failed to get audio decoder element: %x", err);
    } else {
        esp_opus_dec_frame_duration_t frame_duration = ESP_OPUS_DEC_FRAME_DURATION_INVALID;
        if (playback->audio_in_info.frame_duration_ms == 20) {
            frame_duration = ESP_OPUS_DEC_FRAME_DURATION_20_MS;
        } else if (playback->audio_in_info.frame_duration_ms == 60) {
            frame_duration = ESP_OPUS_DEC_FRAME_DURATION_60_MS;
        } else {
            ESP_LOGE(TAG, "Invalid frame duration: %d", playback->audio_in_info.frame_duration_ms);
            return ESP_GMF_ERR_INVALID_ARG;
        }
        esp_opus_dec_cfg_t opus_dec_cfg = {
            .channel = ESP_AUDIO_MONO,
            .frame_duration = frame_duration,
            .self_delimited = false,
            .sample_rate = playback->audio_in_info.sample_rate,
        };
        esp_audio_simple_dec_cfg_t audio_dec_cfg = {
            .dec_type = ESP_AUDIO_TYPE_OPUS,
            .dec_cfg = &opus_dec_cfg,
            .cfg_size = sizeof(opus_dec_cfg),
        };
        err = esp_gmf_audio_dec_reconfig(ele, &audio_dec_cfg);
        if (err != ESP_GMF_ERR_OK) {
            ESP_LOGE(TAG, "Failed to configure OPUS decoder: %x", err);
        }

        ESP_LOGI(TAG, "Configured OPUS decoder pipeline");
    }

    esp_gmf_info_sound_t in_info = {
        .sample_rates = playback->audio_in_info.sample_rate,
        .bits = 16,
        .channels = 1,
        .format_id = ESP_AUDIO_TYPE_OPUS,
    };
    esp_gmf_pipeline_report_info(pipeline_handle, ESP_GMF_INFO_SOUND, &in_info, sizeof(in_info));

    return ESP_GMF_ERR_OK;
}

static esp_gmf_err_t pipeline_setup_ports(esp_gmf_pipeline_handle_t pipeline_handle, const char *input_name, const char *output_name, audio_playback_t *playback)
{
    esp_gmf_err_t err = ESP_GMF_ERR_OK;

    esp_gmf_port_handle_t in_port = NEW_ESP_GMF_PORT_IN_BYTE(
        playback_inport_acquire_read,
        playback_inport_release_read,
        NULL, playback, 2048, portMAX_DELAY);

    err = esp_gmf_pipeline_reg_el_port(pipeline_handle, input_name, ESP_GMF_IO_DIR_READER, in_port);
    if (err != ESP_GMF_ERR_OK) {
        ESP_LOGE(TAG, "Failed to register input port: %x", err);
        return err;
    }

    esp_gmf_port_handle_t out_port = NEW_ESP_GMF_PORT_OUT_BYTE(
        playback_outport_acquire_write,
        playback_outport_release_write,
        NULL, playback, 2048, portMAX_DELAY);

    err = esp_gmf_pipeline_reg_el_port(pipeline_handle, output_name, ESP_GMF_IO_DIR_WRITER, out_port);
    if (err != ESP_GMF_ERR_OK) {
        ESP_LOGE(TAG, "Failed to register output port: %x", err);
        return err;
    }

    return ESP_GMF_ERR_OK;
}

static esp_gmf_pipeline_handle_t pipeline_init(esp_gmf_pool_handle_t pool, audio_playback_t *playback)
{
    esp_gmf_pipeline_handle_t pipeline_handle = NULL;
    const char *el_names[] = {
        "aud_dec",
        "aud_rate_cvt",
        "aud_bit_cvt",
        "aud_ch_cvt",
    };
    size_t num_el = sizeof(el_names) / sizeof(el_names[0]);
    esp_gmf_err_t err = esp_gmf_pool_new_pipeline(pool, NULL, el_names, num_el, NULL, &pipeline_handle);

    err = pipeline_setup_ports(pipeline_handle, el_names[0], el_names[num_el - 1], playback);
    if (err != ESP_GMF_ERR_OK) {
        ESP_LOGE(TAG, "Failed to setup ports: %x", err);
        goto err;
    }
    return pipeline_handle;

err:
    if (pipeline_handle) {
        esp_gmf_pipeline_destroy(pipeline_handle);
    }
    return NULL;
}

static int out_data_callback(uint8_t *data, int data_size, void *ctx)
{
    esp_codec_dev_handle_t dev = (esp_codec_dev_handle_t)ctx;
    esp_codec_dev_write(dev, data, data_size);
    return 0;
}

static int embed_flash_io_set(esp_asp_handle_t *handle, void *ctx)
{
    esp_gmf_pipeline_handle_t pipe = NULL;
    audio_playback_t *playback = (audio_playback_t *)ctx;
    int ret =  esp_audio_simple_player_get_pipeline(handle, &pipe);
    if (pipe) {
        esp_gmf_io_handle_t flash = NULL;
        ret = esp_gmf_pipeline_get_in(pipe, &flash);
        if ((ret == ESP_GMF_ERR_OK) && (strcasecmp(OBJ_GET_TAG(flash), "io_embed_flash") == 0)) {
            embed_item_info_t embed_info = {
                .address = playback->asp_embed_data,
                .size = playback->asp_embed_data_len,
            };
            ret = esp_gmf_io_embed_flash_set_context(flash, &embed_info, 1);
        }
    }
    return ret;
}

static esp_err_t media_playback_init(audio_playback_t *playback)
{
    esp_err_t err = ESP_OK;
    esp_asp_cfg_t asp_cfg = {
        .in = {
            .cb = NULL,
            .user_ctx = NULL
        },
        .out = {
            .cb = out_data_callback,
            .user_ctx = playback->out_dev_handle,
        },
        .prev = embed_flash_io_set,
        .prev_ctx = playback,
    };
    err = esp_audio_simple_player_new(&asp_cfg, &playback->asp_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize audio simple player: %x", err);
    }

    return err;
}

static esp_gmf_task_handle_t pipeline_task_bind_run(esp_gmf_pipeline_handle_t pipeline_handle)
{
    esp_gmf_err_t err = ESP_GMF_ERR_OK;
    esp_gmf_task_handle_t task_handle = NULL;

    esp_gmf_task_cfg_t task_cfg = DEFAULT_ESP_GMF_TASK_CONFIG();
    task_cfg.name = "audio_playback_pipeline";
    task_cfg.thread.stack_in_ext = true;
    task_cfg.thread.stack = 1024 * 40;
    task_cfg.thread.prio = 6;

    err = esp_gmf_task_init(&task_cfg, &task_handle);
    if (err != ESP_GMF_ERR_OK) {
        ESP_LOGE(TAG, "Failed to initialize pipeline task: %x", err);
        goto err;
    }

    err = esp_gmf_pipeline_bind_task(pipeline_handle, task_handle);
    if (err != ESP_GMF_ERR_OK) {
        ESP_LOGE(TAG, "Failed to bind and run pipeline task: %x", err);
        goto err;
    }

    err = esp_gmf_pipeline_loading_jobs(pipeline_handle);
    if (err != ESP_GMF_ERR_OK) {
        ESP_LOGE(TAG, "Failed to load pipeline jobs: %x", err);
        goto err;
    }

    err = esp_gmf_pipeline_run(pipeline_handle);
    if (err != ESP_GMF_ERR_OK) {
        ESP_LOGE(TAG, "Failed to run pipeline: %x", err);
        goto err;
    }

    return task_handle;

err:
    if (task_handle) {
        esp_gmf_task_deinit(task_handle);
    }
    return NULL;
}

void audio_playback_pipeline_restart(void *arg)
{
    audio_playback_t *playback = (audio_playback_t *)arg;
    esp_err_t err = esp_gmf_pipeline_run(playback->pipeline_handle);
    if (err != ESP_GMF_ERR_OK) {
        ESP_LOGE(TAG, "Failed to restart pipeline task");
    } else {
        ESP_LOGI(TAG, "Pipeline task restarted successfully");
    }

    vTaskDelete(NULL);
}

static esp_gmf_err_t audio_playback_event_handler(esp_gmf_event_pkt_t *pkt, void *ctx)
{
    audio_playback_t *playback = (audio_playback_t *)ctx;

    if (pkt == NULL || playback == NULL) {
        return ESP_GMF_ERR_INVALID_ARG;
    }

    // Handle state change events
    if (pkt->type == ESP_GMF_EVT_TYPE_CHANGE_STATE) {
        esp_gmf_event_state_t state = (esp_gmf_event_state_t)pkt->sub;
        ESP_LOGD(TAG, "Pipeline event: state change to %s", esp_gmf_event_get_state_str(state));

        // Detect pipeline errors or stops and trigger restart
        if (state == ESP_GMF_EVENT_STATE_ERROR || state == ESP_GMF_EVENT_STATE_STOPPED) {
            if (playback->started) {
                playback->started = false;
                ESP_LOGW(TAG, "Pipeline entered error/stopped state, triggering restart");

                // Attempt to restart the pipeline immediately
                xTaskCreate(audio_playback_pipeline_restart, "audio_playback_pipeline_restart", 1024 * 4, playback, 6, NULL);
            }
        } else if (state == ESP_GMF_EVENT_STATE_RUNNING) {
            ESP_LOGI(TAG, "Playback pipeline running successfully");
            playback->started = true;
        }
    }

    return ESP_GMF_ERR_OK;
}

audio_playback_handle_t audio_playback_init(const audio_playback_config_t *config)
{
    if (config == NULL || config->out_dev_handle == NULL) {
        ESP_LOGE(TAG, "Invalid config for audio_playback");
        return NULL;
    }

    audio_playback_t *playback = (audio_playback_t *)calloc(1, sizeof(audio_playback_t));
    if (playback == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for audio playback");
        return NULL;
    }

    playback->out_dev_handle = config->out_dev_handle;
    playback->audio_in_info = config->audio_in_info;
    playback->out_codec_info = config->out_codec_info;
    playback->started = false;

    esp_gmf_err_t err = audio_pool_setup();
    if (err != ESP_GMF_ERR_OK) {
        ESP_LOGE(TAG, "Failed to setup shared pool: %x", err);
        goto err;
    }

    esp_gmf_pool_handle_t pool_handle = audio_pool_get();
    if (pool_handle == NULL) {
        ESP_LOGE(TAG, "Failed to get shared pool");
        goto err;
    }

    esp_gmf_err_t fifo_err = esp_gmf_db_new_fifo(AUDIO_PLAYBACK_FIFO_BLOCK_COUNT, 1, &playback->fifo);
    if (fifo_err != ESP_GMF_ERR_OK) {
        ESP_LOGE(TAG, "Failed to create ESP-GMF fifo: %x", fifo_err);
        goto err;
    }

    esp_gmf_pipeline_handle_t pipeline_handle = pipeline_init(pool_handle, playback);
    if (pipeline_handle == NULL) {
        ESP_LOGE(TAG, "Failed to initialize pipeline");
        goto err;
    }
    playback->pipeline_handle = pipeline_handle;

    err = pipeline_setup_elements(playback->pipeline_handle, playback);
    if (err != ESP_GMF_ERR_OK) {
        ESP_LOGE(TAG, "Failed to setup elements: %x", err);
        goto err;
    }

    // Register event handler for pipeline state changes
    err = esp_gmf_pipeline_set_event(playback->pipeline_handle, audio_playback_event_handler, playback);
    if (err != ESP_GMF_ERR_OK) {
        ESP_LOGE(TAG, "Failed to register event handler: %x", err);
        goto err;
    }

    err = media_playback_init(playback);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize media playback: %x", err);
        goto err;
    }

    return (audio_playback_handle_t *)playback;

err:
    if (playback) {
        audio_playback_deinit((audio_playback_handle_t *)playback);
    }
    return NULL;
}

esp_err_t audio_playback_deinit(audio_playback_handle_t *handle)
{
    if (handle == NULL) {
        ESP_LOGE(TAG, "Invalid playback handle");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Deinitializing audio playback");
    audio_playback_t *playback = (audio_playback_t *)handle;

    if (playback->asp_handle) {
        esp_audio_simple_player_destroy(playback->asp_handle);
        playback->asp_handle = NULL;
    }

    if (playback->task_handle) {
        esp_gmf_task_deinit(playback->task_handle);
        playback->task_handle = NULL;
    }

    if (playback->pipeline_handle) {
        esp_gmf_pipeline_destroy(playback->pipeline_handle);
        playback->pipeline_handle = NULL;
    }

    if (playback->fifo) {
        esp_gmf_db_deinit(playback->fifo);
        playback->fifo = NULL;
    }

    free(playback);

    ESP_LOGI(TAG, "Audio playback deinitialized");
    return ESP_OK;
}

esp_err_t audio_playback_start(audio_playback_handle_t *handle)
{
    if (handle == NULL) {
        ESP_LOGE(TAG, "Invalid playback handle");
        return ESP_ERR_INVALID_ARG;
    }

    audio_playback_t *playback = (audio_playback_t *)handle;

    if (playback->started) {
        ESP_LOGW(TAG, "Playback already started");
        return ESP_OK;
    }

    playback->task_handle = pipeline_task_bind_run(playback->pipeline_handle);
    if (playback->task_handle == NULL) {
        ESP_LOGE(TAG, "Failed to start pipeline task");
        return ESP_FAIL;
    }

    playback->started = true;
    ESP_LOGI(TAG, "Audio playback started");

    return ESP_OK;
}

esp_err_t audio_playback_write(audio_playback_handle_t *handle, const uint8_t *data, size_t len)
{
    if (handle == NULL || data == NULL || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    audio_playback_t *playback = (audio_playback_t *)handle;

    if (!playback->started) {
        // ESP_LOGE(TAG, "Playback not started");
        return ESP_ERR_INVALID_STATE;
    }

    esp_gmf_data_bus_block_t blk = {0};

    esp_gmf_err_io_t err = esp_gmf_db_acquire_write(playback->fifo, &blk, len, pdMS_TO_TICKS(100));
    if (err != ESP_GMF_IO_OK) {
        ESP_LOGW(TAG, "Failed to acquire write to ESP-GMF fifo: %x", err);
        return ESP_ERR_TIMEOUT;
    }

    size_t copy_size = len < blk.buf_length ? len : blk.buf_length;
    memcpy(blk.buf, data, copy_size);
    blk.valid_size = copy_size;

    err = esp_gmf_db_release_write(playback->fifo, &blk, pdMS_TO_TICKS(50));
    if (err != ESP_GMF_IO_OK) {
        ESP_LOGW(TAG, "Failed to release write to ESP-GMF fifo: %x", err);
        return ESP_ERR_TIMEOUT;
    }

    ESP_LOGD(TAG, "Sent %d bytes to ESP-GMF speaker data buffer", len);
    return ESP_OK;
}

esp_err_t audio_playback_remaining_bytes(audio_playback_handle_t *handle, size_t *remaining_bytes)
{
    if (handle == NULL || remaining_bytes == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    audio_playback_t *playback = (audio_playback_t *)handle;
    uint32_t filled_size = 0;

    esp_gmf_db_get_filled_size(playback->fifo, &filled_size);

    *remaining_bytes = filled_size;
    return ESP_OK;
}

esp_err_t play_media(audio_playback_handle_t *handle, const char *media_url, const uint8_t *data, size_t len, bool sync)
{
    if (handle == NULL || media_url == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    audio_playback_t *playback = (audio_playback_t *)handle;

    playback->asp_embed_data = data;
    playback->asp_embed_data_len = len;

    ESP_LOGI(TAG, "Playing media: %s", media_url);
    esp_err_t err = ESP_OK;

    if (sync) {
        err = esp_audio_simple_player_run_to_end(playback->asp_handle, media_url, NULL);
    } else {
        err = esp_audio_simple_player_run(playback->asp_handle, media_url, NULL);
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to play media: %x", err);
        return err;
    }

    return err;

}

esp_err_t audio_playback_play_media_sync(audio_playback_handle_t *handle, const char *media_url, const uint8_t *data, size_t len)
{
    return play_media(handle, media_url, data, len, true);
}


esp_err_t audio_playback_play_media_async(audio_playback_handle_t *handle, const char *media_url, const uint8_t *data, size_t len)
{
    return play_media(handle, media_url, data, len, false);
}
