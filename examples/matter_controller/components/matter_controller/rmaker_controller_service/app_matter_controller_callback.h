/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <app_matter_controller.h>

esp_err_t app_matter_controller_callback(matter_controller_handle_t *handle, matter_controller_callback_type_t type);
