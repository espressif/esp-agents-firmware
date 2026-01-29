/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <nvs_flash.h>

#include <esp_rmaker_core.h>
#include <esp_rmaker_standard_types.h>
#include <esp_rmaker_standard_params.h>
#include <esp_rmaker_standard_services.h>
#include <esp_rmaker_utils.h>
#include <esp_event.h>
#include <esp_netif.h>

#include <esp_check.h>

#include <setup/rainmaker.h>
#include <agent_setup.h>
#include <string.h>

static const char *TAG = "setup_rainmaker";

ESP_EVENT_DEFINE_BASE(SETUP_RAINMAKER_EVENT);

#define AGENT_DEVICE_ENABLED CONFIG_AGENT_SETUP_CREATE_RAINMAKER_DEVICE

#ifdef CONFIG_AGENT_SETUP_RAINMAKER_DEVICE_NAME
#define AGENT_DEVICE_NAME        CONFIG_AGENT_SETUP_RAINMAKER_DEVICE_NAME
#else
#define AGENT_DEVICE_NAME        "Espressif AI Agent"
#endif

#define AGENT_ID_PARAM_NAME      "Agent ID"
#define VOLUME_PARAM_NAME        "Volume"

#define AGENT_DEVICE_TYPE "esp.agent"

#define AGENT_PARAM_TYPE_VOLUME "esp.agent.param.volume"
#define AGENT_PARAM_TYPE_AGENT_ID "esp.agent.param.agent-id"

static esp_rmaker_node_t *g_rmaker_node = NULL;
static esp_rmaker_device_t *g_agent_auth_service = NULL;

#if AGENT_DEVICE_ENABLED
static esp_rmaker_device_t *g_agent_device = NULL;

/* Callback function pointer for volume get/set operations */
static esp_err_t (*g_volume_get_cb)(uint8_t *volume) = NULL;
static esp_err_t (*g_volume_set_cb)(uint8_t volume) = NULL;
#endif

static void report_updated_agent_id(void)
{
#if AGENT_DEVICE_ENABLED
    char *agent_id = agent_setup_get_agent_id();
    if (!agent_id) {
        ESP_LOGE(TAG, "Agent ID not found");
        vTaskDelete(NULL);
    }
    esp_rmaker_param_t *param = esp_rmaker_device_get_param_by_name(g_agent_device, AGENT_ID_PARAM_NAME);
    if (param) {
        esp_rmaker_param_update_and_report(param, esp_rmaker_str(agent_id));
    }
#endif /* AGENT_DEVICE_ENABLED */
    /* Don't free agent_id here, as it is a pointer internally managed by agent_setup_data_t */
    vTaskDelete(NULL);
}

static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        esp_rmaker_start();
    }
#if AGENT_DEVICE_ENABLED
    if (event_base == AGENT_SETUP_EVENT && event_id == AGENT_SETUP_EVENT_AGENT_ID_UPDATE) {
        report_updated_agent_id();
    }
#endif
}

static esp_err_t handle_agent_params_update(const char *param_name, const esp_rmaker_param_val_t val)
{
#if AGENT_DEVICE_ENABLED
    esp_err_t err = ESP_OK;
    if (strncmp(param_name, AGENT_ID_PARAM_NAME, strlen(AGENT_ID_PARAM_NAME)) == 0) {
        err = agent_setup_set_agent_id(val.val.s);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to update agent_id: %02x", err);
            goto end;
        }
    } else if (strncmp(param_name, VOLUME_PARAM_NAME, strlen(VOLUME_PARAM_NAME)) == 0) {
        if (g_volume_set_cb) {
            uint8_t volume = (uint8_t)val.val.i;
            err = g_volume_set_cb(volume);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Failed to set volume: %02x", err);
                goto end;
            }
            ESP_LOGI(TAG, "Volume set to %d", volume);
        } else {
            ESP_LOGW(TAG, "Volume update received but set callback not registered");
        }
    }
end:
    return err;

#endif /* AGENT_DEVICE_ENABLED */
    return ESP_OK;
}

static esp_err_t device_bulk_write_cb(const esp_rmaker_device_t *device, const esp_rmaker_param_write_req_t write_req[], uint8_t count, void *priv_data, esp_rmaker_write_ctx_t *ctx)
{
    esp_err_t err = ESP_OK;
    for (uint8_t i = 0; i < count; i++) {
        esp_rmaker_param_t *param = write_req[i].param;
        esp_rmaker_param_val_t val = write_req[i].val;

        const char *param_name = esp_rmaker_param_get_name(param);

        ESP_LOGD(TAG, "Received update for %s", param_name);

        if (strncmp(param_name, ESP_RMAKER_DEF_USER_TOKEN_NAME, strlen(ESP_RMAKER_DEF_USER_TOKEN_NAME)) == 0) {
            if (ctx->src != ESP_RMAKER_REQ_SRC_INIT) {
                err = agent_setup_set_refresh_token(val.val.s);
            }
        } else {
            err = handle_agent_params_update(param_name, val);
        }
    }
    return err;
}

static esp_err_t register_agent_device(void)
{
#if AGENT_DEVICE_ENABLED
    esp_err_t ret = ESP_OK;

    g_agent_device = esp_rmaker_device_create(AGENT_DEVICE_NAME, AGENT_DEVICE_TYPE, NULL);
    ESP_GOTO_ON_FALSE(g_agent_device, ESP_ERR_NO_MEM, end, TAG, "Failed to create %s device", AGENT_DEVICE_NAME);

    uint8_t volume = 0;
    if (g_volume_get_cb) {
        esp_err_t err = g_volume_get_cb(&volume);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Failed to get initial volume: %02x", err);
        }
    }

    const char *agent_id = agent_setup_get_agent_id();
    const char *agent_id_value = agent_id ? agent_id : "";

    esp_rmaker_param_t *name_param = esp_rmaker_name_param_create("Name", AGENT_DEVICE_NAME);
    esp_rmaker_param_t *agent_id_param = esp_rmaker_param_create(AGENT_ID_PARAM_NAME, AGENT_PARAM_TYPE_AGENT_ID, esp_rmaker_str(agent_id_value), PROP_FLAG_READ | PROP_FLAG_WRITE);
    esp_rmaker_param_t *volume_param = esp_rmaker_param_create(VOLUME_PARAM_NAME, AGENT_PARAM_TYPE_VOLUME, esp_rmaker_int(volume), PROP_FLAG_READ | PROP_FLAG_WRITE);

    esp_rmaker_param_add_ui_type(agent_id_param, ESP_RMAKER_UI_TEXT);
    esp_rmaker_param_add_ui_type(volume_param, ESP_RMAKER_UI_SLIDER);
    esp_rmaker_param_add_bounds(volume_param, esp_rmaker_int(0), esp_rmaker_int(100), esp_rmaker_int(1));

    /* FIXME: Change this when phone apps are updated to use new device type */
    // g_agent_device = esp_rmaker_device_create(AGENT_DEVICE_NAME,AGENT_DEVICE_TYPE, NULL);
    g_agent_device = esp_rmaker_device_create(AGENT_DEVICE_NAME, "AI Assistant", NULL);
    ESP_GOTO_ON_FALSE(g_agent_device, ESP_ERR_NO_MEM, end, TAG, "Failed to create %s device", AGENT_DEVICE_NAME);

    esp_rmaker_device_add_param(g_agent_device, agent_id_param);
    esp_rmaker_device_add_param(g_agent_device, name_param);
    esp_rmaker_device_add_param(g_agent_device, volume_param);

    esp_rmaker_device_add_bulk_cb(g_agent_device, device_bulk_write_cb, NULL);
    esp_rmaker_node_add_device(g_rmaker_node, g_agent_device);
end:
    return ret;

#endif /* AGENT_DEVICE_ENABLED */
    return ESP_OK;
}

static esp_err_t add_rmaker_device(void)
{
    esp_err_t ret = ESP_OK;

    /* Create the agent device only if it is enabled in menuconfig */
    ret = register_agent_device();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register agent device: %02x", ret);
    }

    /* Always create the agent auth service */
    g_agent_auth_service = esp_rmaker_create_user_auth_service("AgentAuth", device_bulk_write_cb, NULL, NULL);
    ESP_GOTO_ON_FALSE(g_agent_auth_service, ESP_ERR_NO_MEM, end, TAG, "Failed to create Agent Auth service");
    esp_rmaker_device_add_bulk_cb(g_agent_auth_service, device_bulk_write_cb, NULL);
    esp_rmaker_node_add_device(g_rmaker_node, g_agent_auth_service);

end:
    return ret;
}

esp_err_t setup_rainmaker_init(const char *board_device_manual_url)
{
    esp_rmaker_config_t config = {
        .enable_time_sync = true,
    };
    esp_err_t ret = ESP_OK;

    g_rmaker_node = esp_rmaker_node_init(&config, AGENT_DEVICE_NAME, "AI Agent");
    ESP_GOTO_ON_FALSE(g_rmaker_node, ESP_ERR_NO_MEM, err, TAG, "Failed to initialize RainMaker node");

    esp_rmaker_timezone_service_enable();
    /* Enable OTA */
    esp_rmaker_ota_enable_default();
    /* Enable system service */
    esp_rmaker_system_serv_config_t system_serv_config = {
        .flags = SYSTEM_SERV_FLAGS_ALL,
        .reboot_seconds = 0,
        .reset_seconds = 2,
        .reset_reboot_seconds = 0,
    };
    esp_rmaker_system_service_enable(&system_serv_config);

    if (board_device_manual_url) {
        ret = esp_rmaker_node_add_readme(g_rmaker_node, board_device_manual_url);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to add readme URL: %d", ret);
        }
    }

    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, event_handler, NULL);
    esp_event_handler_register(AGENT_SETUP_EVENT, AGENT_SETUP_EVENT_AGENT_ID_UPDATE, event_handler, NULL);

    ESP_GOTO_ON_ERROR(add_rmaker_device(), err, TAG, "Failed to add RainMaker device");

    return ESP_OK;

err:
    return ret;
}

esp_err_t setup_rainmaker_factory_reset(void)
{
    /* Rainmaker factory reset the device, need send node unbind to the cloud first */
    return esp_rmaker_factory_reset(2, 0);
}

esp_err_t setup_rainmaker_register_volume_callbacks(esp_err_t (*get_cb)(uint8_t *volume), esp_err_t (*set_cb)(uint8_t volume))
{
#if AGENT_DEVICE_ENABLED
    if (!get_cb || !set_cb) {
        return ESP_ERR_INVALID_ARG;
    }
    g_volume_get_cb = get_cb;
    g_volume_set_cb = set_cb;
#endif /* AGENT_DEVICE_ENABLED */
    return ESP_OK;
}

esp_err_t setup_rainmaker_update_volume(uint8_t volume)
{
#if AGENT_DEVICE_ENABLED
    if (!g_agent_device) {
        return ESP_ERR_INVALID_STATE;
    }
    if (volume > 100) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_rmaker_param_t *param = esp_rmaker_device_get_param_by_name(g_agent_device, VOLUME_PARAM_NAME);
    if (param) {
        return esp_rmaker_param_update_and_report(param, esp_rmaker_int(volume));
    }
    return ESP_FAIL;
#endif
    return ESP_OK;
}
