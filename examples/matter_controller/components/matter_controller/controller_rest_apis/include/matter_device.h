/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <esp_err.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define ESP_MATTER_DEVICE_MAX_ENDPOINT 8
#define ESP_MATTER_DEVICE_NAME_MAX_LEN 32
#define ESP_RAINMAKER_NODE_ID_MAX_LEN 36

typedef struct endpoint_entry {
    uint16_t endpoint_id;
    uint32_t device_type_id;
    char device_name[ESP_MATTER_DEVICE_NAME_MAX_LEN];
} endpoint_entry_t;

typedef struct matter_device {
    uint64_t node_id;
    char rainmaker_node_id[ESP_RAINMAKER_NODE_ID_MAX_LEN];
    bool is_metadata_fetched;
    uint8_t endpoint_count;
    endpoint_entry_t endpoints[ESP_MATTER_DEVICE_MAX_ENDPOINT];
    bool reachable;
    bool is_rainmaker_device;
    struct matter_device *next;
} matter_device_t;

/**
 * Free the allocated memory for matter device entries list
 *
 * @param[in] dev_list The device list to free
 */
void free_matter_device_list(matter_device_t *dev_list);

/**
 * Print the informations for matter device entries list
 *
 * @param[in] dev_list The device list to be printed
 */
void print_matter_device_list(matter_device_t *dev_list);

#ifdef __cplusplus
} // extern "C"
#endif
