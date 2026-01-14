/**
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <esp_event.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <inttypes.h>
#include <nvs_flash.h>
#include <string.h>

#ifdef CONFIG_OPENTHREAD_BORDER_ROUTER
#include <app_thread_config.h>
#include <esp_rmaker_thread_br.h>
#include <esp_spiffs.h>
#endif
#include <esp_rmaker_console.h>
#include <esp_rmaker_core.h>
#include <esp_rmaker_ota.h>
#include <esp_rmaker_scenes.h>
#include <esp_rmaker_schedule.h>
#include <esp_rmaker_standard_devices.h>
#include <esp_rmaker_standard_params.h>
#include <esp_rmaker_standard_types.h>

#include <app_matter_device_manager.h>
#include <esp_rmaker_common_events.h>

#include <app_matter_controller.h>
#include <app_matter_controller_callback.h>
#include <app_matter_controller_creds_issuer.h>
#include <esp_matter.h>
#include <esp_matter_console.h>
#include <esp_matter_controller_client.h>
#include <esp_matter_controller_console.h>
#include <esp_matter_controller_credentials_issuer.h>
#include <matter_controller_std.h>

static const char *TAG = "app_controller";

/* Callback to handle commands received from the RainMaker cloud */
static esp_err_t write_cb(const esp_rmaker_device_t *device, const esp_rmaker_param_t *param,
                          const esp_rmaker_param_val_t val, void *priv_data, esp_rmaker_write_ctx_t *ctx)
{
    if (ctx) {
        ESP_LOGI(TAG, "Received write request via : %s", esp_rmaker_device_cb_src_to_str(ctx->src));
    }
    if (strcmp(esp_rmaker_param_get_name(param), ESP_RMAKER_DEF_POWER_NAME) == 0) {
        ESP_LOGI(TAG, "Received value = %s for %s - %s", val.val.b ? "true" : "false",
                 esp_rmaker_device_get_name(device), esp_rmaker_param_get_name(param));
        esp_rmaker_param_update_and_report(param, val);
    }
    return ESP_OK;
}

#if defined(CONFIG_OPENTHREAD_BORDER_ROUTER) && defined(CONFIG_AUTO_UPDATE_RCP)
static esp_err_t init_spiffs()
{
    esp_err_t err = ESP_OK;
    esp_vfs_spiffs_conf_t rcp_fw_conf = {.base_path = "/" CONFIG_RCP_PARTITION_NAME,
                                         .partition_label = CONFIG_RCP_PARTITION_NAME,
                                         .max_files = 10,
                                         .format_if_mount_failed = false};
    err = esp_vfs_spiffs_register(&rcp_fw_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount rcp firmware storage");
    }
    return err;
}
#endif // CONFIG_AUTO_UPDATE_RCP

static void update_controller_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        if (matter_controller_handle_update() == ESP_OK) {
            matter_controller_update_device_list();
            esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &update_controller_handler);
        }
    }
}

static esp_err_t matter_controller_start()
{
    ESP_LOGI(TAG, "Initializing Matter Controller...");
    /* Initialize NVS. */
    ESP_RETURN_ON_ERROR(nvs_flash_init(), TAG, "Failed to initialize NVS");

#if defined(CONFIG_OPENTHREAD_BORDER_ROUTER) && defined(CONFIG_AUTO_UPDATE_RCP)
    ESP_RETURN_ON_ERROR(init_spiffs(), TAG, "Failed to initialize SPIFFS");
#endif

    esp_rmaker_config_t rainmaker_cfg = {
        .enable_time_sync = false,
    };
    const esp_rmaker_node_t *node = esp_rmaker_node_init(&rainmaker_cfg, "ESP RainMaker Device", "Controller");
    if (!node) {
        /* try to get node */
        node = esp_rmaker_get_node();
        if (!node) {
            ESP_RETURN_ON_ERROR(ESP_ERR_INVALID_STATE, TAG, "Could not initialise node or get node.");
        }
    } else {
        /* TODO: The service will be showed on every device on the node now. If the node has been initialized,
           we don't create the device and just add the service to the node */
        /* Create a MatterController device. */
        esp_rmaker_device_t *matter_controller_device =
            esp_rmaker_device_create("MatterController", ESP_RMAKER_DEVICE_MATTER_CONTROLLER, NULL);

        /* Add the write callback for the device. We aren't registering any read callback yet as
        * it is for future use.
        */
        esp_rmaker_device_add_cb(matter_controller_device, write_cb, NULL);

        /* Add the standard name parameter (type: esp.param.name), which allows setting a persistent,
        * user friendly custom name from the phone apps. All devices are recommended to have this
        * parameter.
        */
        esp_rmaker_device_add_param(matter_controller_device,
                                    esp_rmaker_name_param_create(ESP_RMAKER_DEF_NAME_PARAM, "MatterController"));

        /* Add this Matter controller device to the node */
        esp_rmaker_node_add_device(node, matter_controller_device);
    }

#ifdef CONFIG_OPENTHREAD_BORDER_ROUTER
    /* Enable Thread Border Router */
    esp_openthread_platform_config_t thread_cfg = {
        .radio_config = ESP_OPENTHREAD_DEFAULT_RADIO_CONFIG(),
        .host_config = ESP_OPENTHREAD_DEFAULT_HOST_CONFIG(),
        .port_config = ESP_OPENTHREAD_DEFAULT_PORT_CONFIG(),
    };
#ifdef CONFIG_AUTO_UPDATE_RCP
    esp_rcp_update_config_t rcp_update_cfg = ESP_OPENTHREAD_RCP_UPDATE_CONFIG();
    esp_rmaker_thread_br_enable(&thread_cfg, &rcp_update_cfg);
#else
    esp_rmaker_thread_br_enable(&thread_cfg);
#endif /* CONFIG_AUTO_UPDATE_RCP */
#endif /* CONFIG_OPENTHREAD_BORDER_ROUTER */

    /* Enable Matter Controller service */
    matter_controller_enable(CONFIG_ESP_MATTER_CONTROLLER_VENDOR_ID, app_matter_controller_callback);

    /* Start matter */
    ESP_RETURN_ON_ERROR(esp_matter::start(NULL), TAG, "Failed to start Matter");
    esp_matter::lock::chip_stack_lock(portMAX_DELAY);
    ESP_RETURN_ON_ERROR(esp_matter::controller::matter_controller_client::get_instance().init(0, 0, 5580), TAG, "Failed to initialize Matter Controller Client");
    esp_matter::lock::chip_stack_unlock();
    ESP_RETURN_ON_ERROR(init_device_manager(NULL), TAG, "Failed to initialize Device Manager");
    /* Update matter controller after joining to Wi-Fi network */
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &update_controller_handler, NULL);
    return ESP_OK;
}

static void matter_controller_launcher_task(void *pvParameters)
{
    esp_err_t err = matter_controller_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Matter controller start failed: %s", esp_err_to_name(err));
    }
    vTaskDelete(NULL);
}

extern "C" esp_err_t matter_controller_start_task(void)
{
    if (xTaskCreate(matter_controller_launcher_task, "launch_matter", 5120, NULL, 6, NULL) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to create Matter launcher task");
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}
