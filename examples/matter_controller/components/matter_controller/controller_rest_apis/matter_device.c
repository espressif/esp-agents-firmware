/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <esp_log.h>
#include <matter_device.h>

#define TAG "MATTER_DEVICE"

void free_matter_device_list(matter_device_t *dev_list)
{
    matter_device_t *current = dev_list;
    while(current) {
        dev_list = dev_list->next;
        free(current);
        current = dev_list;
    }
}

void print_matter_device_list(matter_device_t *dev_list)
{
    uint16_t dev_index = 0;
    while (dev_list) {
        ESP_LOGI(TAG, "device %d : {", dev_index);
        ESP_LOGI(TAG, "    rainmaker_node_id: %s,", dev_list->rainmaker_node_id);
        ESP_LOGI(TAG, "    matter_node_id: 0x%llX,", dev_list->node_id);
        if (dev_list->is_metadata_fetched) {
            ESP_LOGI(TAG, "    is_rainmaker_device: %s,", dev_list->is_rainmaker_device ? "true" : "false");
            ESP_LOGI(TAG, "    is_online: %s,", dev_list->reachable ? "true" : "false");
            ESP_LOGI(TAG, "    endpoints : [");
            for (size_t i = 0; i < dev_list->endpoint_count; ++i) {
                ESP_LOGI(TAG, "        {");
                ESP_LOGI(TAG, "           endpoint_id: %d,", dev_list->endpoints[i].endpoint_id);
                ESP_LOGI(TAG, "           device_type_id: 0x%lx,", dev_list->endpoints[i].device_type_id);
                ESP_LOGI(TAG, "           device_name: %s,", dev_list->endpoints[i].device_name);
                ESP_LOGI(TAG, "        },");
            }
            ESP_LOGI(TAG, "    ]");
        }
        ESP_LOGI(TAG, "}");
        dev_list = dev_list->next;
        dev_index++;
    }
}
