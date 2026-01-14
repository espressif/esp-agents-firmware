/**
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_audio_dec_default.h"
#include "esp_audio_enc_default.h"
#include "sdkconfig.h"
#include <esp_log.h>
#include <esp_gmf_pool.h>
#include <esp_afe_sr_models.h>
#include <esp_afe_config.h>
#include <esp_gmf_afe.h>
#include <esp_gmf_afe_manager.h>

#include <esp_gmf_audio_enc.h>
#include <esp_gmf_audio_dec.h>

#include <esp_gmf_rate_cvt.h>
#include <esp_gmf_bit_cvt.h>
#include <esp_gmf_ch_cvt.h>

#include <audio_common.h>

typedef struct {
    esp_gmf_pool_handle_t pool_handle;
    bool initialized;
} audio_common_data_t;

static const char *TAG = "audio_common";

audio_common_data_t g_audio_common_data;

static esp_gmf_err_t pool_register_converters(esp_gmf_pool_handle_t pool)
{
    esp_gmf_element_handle_t rate_cvt = NULL;
    esp_gmf_err_t err = ESP_GMF_ERR_OK;

    esp_gmf_rate_cvt_init(NULL, &rate_cvt);
    err = esp_gmf_pool_register_element(pool, rate_cvt, NULL);
    if (err != ESP_GMF_ERR_OK) {
        ESP_LOGW(TAG, "Failed to register rate cvt: %x", err);
    }

    esp_gmf_element_handle_t bit_cvt = NULL;
    esp_gmf_bit_cvt_init(NULL, &bit_cvt);
    err = esp_gmf_pool_register_element(pool, bit_cvt, NULL);
    if (err != ESP_GMF_ERR_OK) {
        ESP_LOGW(TAG, "Failed to register bit cvt: %x", err);
    }

    esp_gmf_element_handle_t ch_cvt = NULL;
    esp_gmf_ch_cvt_init(NULL, &ch_cvt);
    err = esp_gmf_pool_register_element(pool, ch_cvt, NULL);
    if (err != ESP_GMF_ERR_OK) {
        ESP_LOGW(TAG, "Failed to register ch cvt: %x", err);
    }

    esp_gmf_element_handle_t audio_enc = NULL;
    err = esp_gmf_audio_enc_init(NULL, &audio_enc);
    if (err != ESP_GMF_ERR_OK) {
        ESP_LOGW(TAG, "Failed to initialize audio enc: %x", err);
    }

    err = esp_gmf_pool_register_element(pool, audio_enc, NULL);
    if (err != ESP_GMF_ERR_OK) {
        ESP_LOGW(TAG, "Failed to register audio enc: %x", err);
    }

    err = esp_audio_enc_register_default();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to register audio encoders");
    }

    esp_gmf_element_handle_t audio_dec = NULL;
    err = esp_gmf_audio_dec_init(NULL, &audio_dec);
    if (err != ESP_GMF_ERR_OK) {
        ESP_LOGW(TAG, "Failed to initialize audio dec: %x", err);
    }

    err = esp_gmf_pool_register_element(pool, audio_dec, NULL);
    if (err != ESP_GMF_ERR_OK) {
        ESP_LOGW(TAG, "Failed to register audio dec: %x", err);
    }

    err = esp_audio_dec_register_default();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to register audio decoders");
    }

    return ESP_GMF_ERR_OK;
}

static esp_gmf_afe_manager_handle_t pool_setup_afe(const char *input_format, esp_gmf_pool_handle_t pool)
{
    esp_gmf_err_t err = ESP_GMF_ERR_OK;

    void *models = esp_srmodel_init("model");
    afe_config_t *afe_cfg = afe_config_init(input_format, models, AFE_TYPE_SR, AFE_MODE_HIGH_PERF);
    afe_cfg->wakenet_init = true;
#if CONFIG_ENABLE_AEC
    afe_cfg->aec_init = true;
#else
    afe_cfg->aec_init = false;
#endif
    afe_cfg->vad_init = true;
    afe_cfg->vad_min_noise_ms = 1000;
    afe_cfg->vad_min_speech_ms = 64;
    afe_cfg->vad_mode = VAD_MODE_3;
    afe_cfg->se_init = false;
    afe_cfg->agc_init = true;
    afe_cfg->memory_alloc_mode = AFE_MEMORY_ALLOC_MORE_PSRAM;

    esp_gmf_afe_manager_cfg_t gmf_afe_cfg = DEFAULT_GMF_AFE_MANAGER_CFG(afe_cfg, NULL, NULL, NULL, NULL);
    gmf_afe_cfg.feed_task_setting.core = 1;
    gmf_afe_cfg.feed_task_setting.prio = 6;
    gmf_afe_cfg.feed_task_setting.stack_size = 5*1024;

    gmf_afe_cfg.fetch_task_setting.core = 0;
    gmf_afe_cfg.fetch_task_setting.prio = 6;
    gmf_afe_cfg.fetch_task_setting.stack_size = 5*1024;

    esp_gmf_afe_manager_handle_t gmf_afe_manager = NULL;
    err = esp_gmf_afe_manager_create(&gmf_afe_cfg, &gmf_afe_manager);

    if (err != ESP_GMF_ERR_OK) {
        ESP_LOGE(TAG, "Failed to initialize gmf afe manager: %d", err);
        return NULL;
    }

    esp_gmf_element_handle_t ai_afe = NULL;
    esp_gmf_afe_cfg_t ai_afe_cfg = DEFAULT_GMF_AFE_CFG(gmf_afe_manager, NULL, NULL, models);
    ai_afe_cfg.wakeup_end = 15 * 1000; // 15 seconds

    err = esp_gmf_afe_init(&ai_afe_cfg, &ai_afe);
    if (err != ESP_GMF_ERR_OK) {
        ESP_LOGE(TAG, "Failed to initialize ai afe: %x", err);
        goto err;
    }
    err = esp_gmf_pool_register_element(pool, ai_afe, "ai_afe");
    if (err != ESP_GMF_ERR_OK) {
        ESP_LOGE(TAG, "Failed to register ai afe: %x", err);
        goto err;
    }

    return gmf_afe_manager;

err:
   if (gmf_afe_manager) {
        esp_gmf_afe_manager_destroy(gmf_afe_manager);
    }
    return NULL;
}

esp_err_t audio_pool_setup(void)
{
    if (g_audio_common_data.initialized) {
        return ESP_OK;
    }

    // Initialize GMF pool
    esp_gmf_err_t err = esp_gmf_pool_init(&g_audio_common_data.pool_handle);
    if (err != ESP_GMF_ERR_OK) {
        ESP_LOGE(TAG, "Failed to initialize pool: %d", err);
        return ESP_FAIL;
    }

    // Register common elements in the pool
    err = pool_register_converters(g_audio_common_data.pool_handle);
    if (err != ESP_GMF_ERR_OK) {
        ESP_LOGE(TAG, "Failed to register converters: %x", err);
        esp_gmf_pool_deinit(g_audio_common_data.pool_handle);
        g_audio_common_data.pool_handle = NULL;
        return ESP_FAIL;
    }

    g_audio_common_data.initialized = true;

    return ESP_OK;
}

esp_gmf_pool_handle_t audio_pool_get(void)
{
    return g_audio_common_data.pool_handle;
}

void* audio_pool_register_afe(const char *input_format)
{
    if (!g_audio_common_data.initialized || !g_audio_common_data.pool_handle) {
        ESP_LOGE(TAG, "Audio pool not initialized");
        return NULL;
    }

    return pool_setup_afe(input_format, g_audio_common_data.pool_handle);
}
