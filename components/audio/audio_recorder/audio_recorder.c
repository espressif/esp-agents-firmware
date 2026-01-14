/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_audio_enc.h"
#include "esp_opus_enc.h"
#include <esp_check.h>
#include <esp_log.h>

#include <esp_gmf_pool.h>
#include <esp_gmf_pipeline.h>

#include <esp_afe_sr_models.h>
#include <esp_afe_config.h>
#include <esp_gmf_afe.h>
#include <esp_gmf_afe_manager.h>

#include <esp_gmf_audio_enc.h>
#include <esp_gmf_rate_cvt.h>
#include <esp_gmf_ch_cvt.h>
#include <esp_gmf_bit_cvt.h>

#include <esp_gmf_fifo.h>

#include <audio_common.h>
#include <audio_recorder.h>

static const char *TAG = "audio_recorder";

#define AUDIO_RECORDER_FIFO_BLOCK_COUNT 8

typedef struct {
    esp_gmf_pipeline_handle_t pipeline_handle;
    esp_codec_dev_handle_t in_dev_handle;
    esp_gmf_task_handle_t task_handle;
    esp_gmf_fifo_handle_t fifo_handle;
    uint16_t sample_rate;
    uint8_t frame_duration_ms;
    audio_recorder_event_cb_t event_cb;
    void *cb_user_data;
} audio_recorder_t;

static void esp_gmf_afe_event_cb(esp_gmf_obj_handle_t obj, esp_gmf_afe_evt_t *event, void *user_data)
{
    audio_recorder_event_t recorder_event = AUDIO_RECORDER_EVENT_MAX;
    audio_recorder_t *recorder = (audio_recorder_t *)user_data;

    switch (event->type) {
    case ESP_GMF_AFE_EVT_WAKEUP_START: {
        esp_gmf_afe_wakeup_info_t *info = event->event_data;
        recorder_event = AUDIO_RECORDER_EVENT_WAKEUP_START;
        ESP_LOGI(TAG, "Wakeup start", info->wake_word_index, info->wakenet_model_index);
        break;
    }
    case ESP_GMF_AFE_EVT_WAKEUP_END:
        recorder_event = AUDIO_RECORDER_EVENT_WAKEUP_END;
        ESP_LOGI(TAG, "Wakeup end");
        break;
    case ESP_GMF_AFE_EVT_VAD_START:
        recorder_event = AUDIO_RECORDER_EVENT_VAD_START;
        // ESP_LOGI(TAG, "VAD_START");
        break;
    case ESP_GMF_AFE_EVT_VAD_END:
        recorder_event = AUDIO_RECORDER_EVENT_VAD_END;
        // ESP_LOGI(TAG, "VAD_END");
        break;
    default:
        ESP_LOGW(TAG, "Unknown event: %d", event->type);
        break;
    }

    if (recorder && recorder->event_cb) {
        recorder->event_cb(((audio_recorder_handle_t) recorder), recorder_event, recorder->cb_user_data);
    }
}

static esp_gmf_err_io_t recorder_outport_acquire_write(void *handle, esp_gmf_data_bus_block_t *blk, int wanted_size, int block_ticks)
{
    return ESP_GMF_ERR_OK;
}

static esp_gmf_err_io_t recorder_outport_release_write(void *handle, esp_gmf_data_bus_block_t *blk, int block_ticks)
{
    audio_recorder_t *recorder = (audio_recorder_t *)handle;
    esp_gmf_data_bus_block_t _blk = {0};
    int ret = esp_gmf_fifo_acquire_write(recorder->fifo_handle, &_blk, blk->valid_size, block_ticks);
    if (ret < 0) {
        ESP_LOGE(TAG, "%s|%d, Fifo acquire write failed, ret: %d", __func__, __LINE__, ret);
        return ESP_FAIL;
    }
    memcpy(_blk.buf, blk->buf, blk->valid_size);

    _blk.valid_size = blk->valid_size;
    ret = esp_gmf_fifo_release_write(recorder->fifo_handle, &_blk, block_ticks);
    if (ret != ESP_GMF_ERR_OK) {
        ESP_LOGE(TAG, "Fifo release write failed");
    }
    return ESP_GMF_IO_OK;
}

static esp_gmf_err_io_t recorder_inport_acquire_read(void *handle, esp_gmf_data_bus_block_t *blk, int wanted_size, int block_ticks)
{
    audio_recorder_t *recorder = (audio_recorder_t *)handle;

    esp_codec_dev_read(recorder->in_dev_handle, blk->buf, wanted_size);
    blk->valid_size = wanted_size;

    return ESP_GMF_IO_OK;
}

static esp_gmf_err_io_t recorder_inport_release_read(void *handle, esp_gmf_data_bus_block_t *blk, int block_ticks)
{
    return ESP_GMF_IO_OK;
}


static esp_gmf_err_t pipeline_setup_elements(esp_gmf_pipeline_handle_t pipeline_handle, audio_recorder_handle_t recorder_handle)
{
    esp_gmf_err_t err = ESP_GMF_ERR_OK;
    esp_gmf_element_handle_t ele = NULL;
    audio_recorder_t *recorder = (audio_recorder_t *)recorder_handle;

    err = esp_gmf_pipeline_get_el_by_name(pipeline_handle, "aud_enc", &ele);
    if (err != ESP_GMF_ERR_OK) {
        ESP_LOGE(TAG, "Failed to get audio enc element: %x", err);
        return err;
    }

    esp_opus_enc_frame_duration_t frame_duration = ESP_OPUS_ENC_FRAME_DURATION_ARG;
    if (recorder->frame_duration_ms == 20) {
        frame_duration = ESP_OPUS_ENC_FRAME_DURATION_20_MS;
    } else if (recorder->frame_duration_ms == 60) {
        frame_duration = ESP_OPUS_ENC_FRAME_DURATION_60_MS;
    } else {
        ESP_LOGE(TAG, "Invalid frame duration: %d", recorder->frame_duration_ms);
        return ESP_GMF_ERR_INVALID_ARG;
    }
    esp_opus_enc_config_t opus_enc_cfg = ESP_OPUS_ENC_CONFIG_DEFAULT();
    opus_enc_cfg.application_mode = ESP_OPUS_ENC_APPLICATION_VOIP;
    opus_enc_cfg.frame_duration = frame_duration;
    opus_enc_cfg.sample_rate = recorder->sample_rate;
    opus_enc_cfg.channel = 1;
    opus_enc_cfg.bits_per_sample = 16;

    esp_audio_enc_config_t enc_config = {
        .type = ESP_AUDIO_TYPE_OPUS,
        .cfg = &opus_enc_cfg,
        .cfg_sz = sizeof(esp_opus_enc_config_t),
    };
    err = esp_gmf_audio_enc_reconfig(ele, &enc_config);
    if (err != ESP_GMF_ERR_OK) {
        ESP_LOGE(TAG, "Failed to initialize audio enc: %x", err);
        return err;
    }

    err = esp_gmf_pipeline_get_el_by_name(pipeline_handle, "ai_afe", &ele);
    if (err != ESP_GMF_ERR_OK) {
        ESP_LOGE(TAG, "Failed to get ai afe element: %x", err);
        return err;
    }
    err = esp_gmf_afe_set_event_cb(ele, esp_gmf_afe_event_cb, recorder_handle);
    if (err != ESP_GMF_ERR_OK) {
        ESP_LOGE(TAG, "Failed to set ai afe event callback: %x", err);
        return err;
    }

    esp_gmf_info_sound_t in_info = {
        .sample_rates = 16000,
        .bits = 16,
        .channels = 1,
    };
    esp_gmf_pipeline_report_info(pipeline_handle, ESP_GMF_INFO_SOUND, &in_info, sizeof(in_info));

    return ESP_GMF_ERR_OK;
}

static esp_gmf_task_handle_t pipeline_task_bind_run(esp_gmf_pipeline_handle_t pipeline_handle)
{
    esp_gmf_err_t err = ESP_GMF_ERR_OK;
    esp_gmf_task_handle_t task_handle = NULL;

    esp_gmf_task_cfg_t task_cfg =  DEFAULT_ESP_GMF_TASK_CONFIG();
    task_cfg.name = "audio_rec_pipeline";
    task_cfg.thread.stack_in_ext = true;
    task_cfg.thread.stack = 32 * 1024;
    task_cfg.thread.core = 1;

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


    err = esp_gmf_task_run(task_handle);
    return task_handle;

err:
    if (task_handle) {
        esp_gmf_task_deinit(task_handle);
    }
    return NULL;
}

static esp_gmf_err_t pipeline_setup_ports(esp_gmf_pipeline_handle_t pipeline_handle, const char *input_name, const char *output_name, audio_recorder_handle_t recorder_handle)
{
    esp_gmf_err_t err = ESP_GMF_ERR_OK;
    esp_gmf_port_handle_t out_port = NEW_ESP_GMF_PORT_OUT_BYTE(
        recorder_outport_acquire_write,
        recorder_outport_release_write,
        NULL, recorder_handle, 2048, portMAX_DELAY);

    esp_gmf_err_t ret = esp_gmf_pipeline_reg_el_port(pipeline_handle, output_name, ESP_GMF_IO_DIR_WRITER, out_port);
    if (ret != ESP_GMF_ERR_OK) {
        ESP_LOGE(TAG, "Failed to register output port: %x", ret);
        return ret;
    }

    esp_gmf_port_handle_t in_port = NEW_ESP_GMF_PORT_IN_BYTE(
        recorder_inport_acquire_read,
        recorder_inport_release_read,
        NULL, recorder_handle, 2048, portMAX_DELAY);

    err = esp_gmf_pipeline_reg_el_port(pipeline_handle, input_name, ESP_GMF_IO_DIR_READER, in_port);
    if (err != ESP_GMF_ERR_OK) {
        ESP_LOGE(TAG, "Failed to register input port: %x", err);
        return err;
    }

    return ESP_GMF_ERR_OK;
}

esp_gmf_pipeline_handle_t pipeline_init(esp_gmf_pool_handle_t pool, audio_recorder_handle_t recorder_handle)
{
    esp_gmf_pipeline_handle_t pipeline_handle = NULL;
    const char *el_names[] = {
        "ai_afe",
        "aud_enc",
    };
    size_t num_el = sizeof(el_names) / sizeof(el_names[0]);
    esp_gmf_err_t err = esp_gmf_pool_new_pipeline(pool,NULL, el_names, num_el, NULL, &pipeline_handle);

    err = pipeline_setup_ports(pipeline_handle, el_names[0], el_names[num_el - 1], recorder_handle);
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

audio_recorder_handle_t audio_recorder_init(const audio_recorder_config_t *config)
{
    if (config == NULL || config->format == NULL || config->in_dev_handle == NULL) {
        ESP_LOGE(TAG, "Invalid config for audio_recorder");
        return NULL;
    }

    audio_recorder_t *recorder = (audio_recorder_t *)calloc(1, sizeof(audio_recorder_t));
    if (recorder == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for audio recorder");
        goto err;
    }

    // Store codec device handle
    recorder->in_dev_handle = config->in_dev_handle;
    recorder->sample_rate = config->sample_rate;
    recorder->frame_duration_ms = config->frame_duration_ms;

    esp_gmf_err_t err = audio_pool_setup();
    if (err != ESP_GMF_ERR_OK) {
        ESP_LOGE(TAG, "Failed to setup shared pool: %x", err);
        goto err;
    }
    esp_gmf_pool_handle_t pool_handle = audio_pool_get();
    if (pool_handle == NULL) {
        ESP_LOGE(TAG, "Failed to get shared pool - ensure audio_pool_setup() was called");
        goto err;
    }

    esp_gmf_afe_manager_handle_t gmf_afe_manager = (esp_gmf_afe_manager_handle_t)audio_pool_register_afe(config->format);
    if (gmf_afe_manager == NULL) {
        ESP_LOGE(TAG, "Failed to register AFE to shared pool");
        goto err;
    }

    esp_gmf_pipeline_handle_t pipeline_handle = pipeline_init(pool_handle, (audio_recorder_handle_t)recorder);
    if (pipeline_handle == NULL) {
        ESP_LOGE(TAG, "Failed to initialize pipeline");
        goto err;
    }
    recorder->pipeline_handle = pipeline_handle;

    err = esp_gmf_fifo_create(AUDIO_RECORDER_FIFO_BLOCK_COUNT, 1, &recorder->fifo_handle);
    if (err != ESP_GMF_ERR_OK) {
        ESP_LOGE(TAG, "Failed to initialize FIFO: %x", err);
        goto err;
    }

    err = pipeline_setup_elements(recorder->pipeline_handle, (audio_recorder_handle_t)recorder);
    if (err != ESP_GMF_ERR_OK) {
        ESP_LOGE(TAG, "Failed to setup elements: %x", err);
        goto err;
    }

    return (audio_recorder_handle_t)recorder;

err:
    if (recorder) {
        audio_recorder_deinit((audio_recorder_handle_t)recorder);
    }
    return NULL;
}

esp_err_t audio_recorder_deinit(audio_recorder_handle_t handle)
{
    if (handle == NULL) {
        ESP_LOGE(TAG, "Invalid recorder handle");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Deinitializing audio recorder");
    audio_recorder_t *recorder = (audio_recorder_t *)handle;

    if (recorder->task_handle) {
        esp_gmf_task_deinit(recorder->task_handle);
        recorder->task_handle = NULL;
    }

    if (recorder->pipeline_handle) {
        esp_gmf_pipeline_destroy(recorder->pipeline_handle);
        recorder->pipeline_handle = NULL;
    }

    if (recorder->fifo_handle) {
        esp_gmf_fifo_destroy(recorder->fifo_handle);
        recorder->fifo_handle = NULL;
    }

    free(recorder);

    ESP_LOGI(TAG, "Audio recorder deinitialized");
    return ESP_OK;
}

esp_err_t audio_recorder_start(audio_recorder_handle_t handle)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    audio_recorder_t *recorder = (audio_recorder_t *)handle;
    recorder->task_handle = pipeline_task_bind_run(recorder->pipeline_handle);

    return ESP_OK;
}

esp_err_t audio_recorder_read(audio_recorder_handle_t handle, uint8_t *data, size_t len, size_t *read_len)
{
    if (handle == NULL || data == NULL || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = ESP_OK;

    audio_recorder_t *recorder = (audio_recorder_t *)handle;
    esp_gmf_data_bus_block_t blk;

    err = esp_gmf_fifo_acquire_read(recorder->fifo_handle, &blk, len, portMAX_DELAY);
    if (err != ESP_GMF_IO_OK) {
        *read_len = 0;
        return err;
    }

    size_t copy_len = (blk.valid_size < len) ? blk.valid_size : len;
    memcpy(data, blk.buf, copy_len);
    *read_len = copy_len;

    esp_gmf_fifo_release_read(recorder->fifo_handle, &blk, portMAX_DELAY);

    return ESP_OK;
}

esp_err_t audio_recorder_add_event_cb(audio_recorder_handle_t handle, audio_recorder_event_cb_t cb, void *user_data)
{
    if (!handle || !cb) {
        return ESP_ERR_INVALID_ARG;
    }

    audio_recorder_t *recorder = (audio_recorder_t *)handle;

    recorder->event_cb = cb;
    recorder->cb_user_data = user_data;

    return ESP_OK;
}

esp_err_t audio_recorder_stay_awake(audio_recorder_handle_t handle, bool awake)
{
    if (handle == NULL) {
        ESP_LOGE(TAG, "Invalid recorder handle");
        return ESP_ERR_INVALID_ARG;
    }

    audio_recorder_t *recorder = (audio_recorder_t *)handle;

    if (recorder->pipeline_handle == NULL) {
        ESP_LOGE(TAG, "Pipeline handle not available");
        return ESP_ERR_INVALID_STATE;
    }

    esp_gmf_element_handle_t afe_element = NULL;
    esp_gmf_err_t err = esp_gmf_pipeline_get_el_by_name(recorder->pipeline_handle, "ai_afe", &afe_element);
    if (err != ESP_GMF_ERR_OK) {
        ESP_LOGE(TAG, "Failed to get AFE element from pipeline: %x", err);
        return ESP_FAIL;
    }

    err = esp_gmf_afe_keep_awake(afe_element, awake);
    if (err != ESP_GMF_ERR_OK) {
        ESP_LOGE(TAG, "Failed to set AFE keep awake mode: %x", err);
        return ESP_FAIL;
    }

    ESP_LOGD(TAG, "AFE keep awake mode %s", awake ? "enabled" : "disabled");
    return ESP_OK;
}

esp_err_t audio_recorder_trigger_sleep(audio_recorder_handle_t handle)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    audio_recorder_t *recorder = (audio_recorder_t *)handle;

    esp_gmf_element_handle_t afe_element = NULL;
    esp_gmf_err_t err = esp_gmf_pipeline_get_el_by_name(recorder->pipeline_handle, "ai_afe", &afe_element);
    if (err != ESP_GMF_ERR_OK) {
        ESP_LOGE(TAG, "Failed to get AFE element from pipeline: %x", err);
        return ESP_FAIL;
    }

    err = esp_gmf_afe_trigger_sleep(afe_element);
    if (err != ESP_GMF_ERR_OK) {
        ESP_LOGE(TAG, "Failed to trigger AFE sleep: %x", err);
        return ESP_FAIL;
    }

    ESP_LOGD(TAG, "AFE sleep triggered");
    return ESP_OK;
}
