/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <esp_err.h>
#include <esp_console.h>

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t agent_console_init(void);

esp_err_t agent_console_register_default_commands(void);

esp_err_t agent_console_register_command(const esp_console_cmd_t *cmd);

#ifdef __cplusplus
}
#endif
