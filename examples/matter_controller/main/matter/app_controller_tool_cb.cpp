/**
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "app_matter_device_manager.h"
#include "cJSON.h"
#include "esp_log.h"
#include "core/DataModelTypes.h"

#include <esp_matter_controller_cluster_command.h>

static matter_device_t *s_dev_list = NULL;
using namespace chip::app::Clusters;
#define TAG "app_controller_tool_cb"

extern "C" esp_err_t matter_controller_get_device_list(char **device_list_json)
{
    cJSON *tool_response_json = NULL;

    if (s_dev_list != NULL) {
        free_matter_device_list(s_dev_list);
        s_dev_list = NULL;
    }
    s_dev_list = fetch_device_list();
    tool_response_json = cJSON_CreateObject();
    if (tool_response_json == NULL) {
        ESP_LOGE(TAG, "Failed to create cJSON object");
        return ESP_ERR_NO_MEM;
    }
    cJSON *device_list_array = cJSON_CreateArray();
    if (device_list_array == NULL) {
        ESP_LOGE(TAG, "Failed to create cJSON array");
        cJSON_Delete(tool_response_json);
        return ESP_ERR_NO_MEM;
    }
    matter_device_t *current = s_dev_list;
    while (current) {
        if (current->endpoint_count != 1) {
            current = current->next;
            continue;
        }
        cJSON *device_json = cJSON_CreateObject();
        if (device_json == NULL) {
            ESP_LOGE(TAG, "Failed to create cJSON object for device");
            cJSON_Delete(tool_response_json);
            return ESP_ERR_NO_MEM;
        }
        char node_id_str[17] = {0};
        snprintf(node_id_str, sizeof(node_id_str), "%016llX", current->node_id);
        cJSON_AddStringToObject(device_json, "node_id", node_id_str);
        cJSON_AddStringToObject(device_json, "device_name", current->endpoints[0].device_name);
        cJSON_AddItemToArray(device_list_array, device_json);
        current = current->next;
    }
    cJSON_AddItemToObject(tool_response_json, "device_list", device_list_array);

    *device_list_json = cJSON_PrintUnformatted(tool_response_json);
    cJSON_Delete(tool_response_json);
    return ESP_OK;
}

extern "C" esp_err_t matter_controller_control_device(char **result, uint64_t node_id, uint32_t cluster_id,
                                                      uint32_t command_id, const char *command_params_json)
{
    uint32_t node_id_upper = (uint32_t)(node_id >> 32);
    uint32_t node_id_lower = (uint32_t)(node_id & 0xFFFFFFFF);

    char *msg = NULL;
    if (command_params_json != NULL) {
        msg = strdup(command_params_json);
    }
    chip::DeviceLayer::SystemLayer().ScheduleLambda([node_id_upper, node_id_lower, cluster_id, command_id, msg]() {
        esp_matter::controller::send_invoke_cluster_command(uint64_t(node_id_upper) << 32 | node_id_lower, 1,
                                                            cluster_id, command_id, msg);
        free(msg);
    });

    *result = strdup("Command sent successfully.");
    return ESP_OK;
}
