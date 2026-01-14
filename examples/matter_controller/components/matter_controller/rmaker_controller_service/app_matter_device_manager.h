/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <matter_device.h>
#include <stdint.h>

#include <app_matter_controller.h>

typedef void (*device_list_update_callback_t)(void);

esp_err_t update_device_list(matter_controller_handle_t *controller_handle);

matter_device_t *fetch_device_list();

esp_err_t init_device_manager(device_list_update_callback_t dev_list_update_cb);
