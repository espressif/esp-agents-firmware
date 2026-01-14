/**
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Start the Matter controller
 *
 * It relies on RainMaker service, so should be called after `setup_rainmaker_start`
 *
 * @return ESP_OK on success, otherwise an error code
 */
esp_err_t matter_controller_start_task(void);

esp_err_t matter_controller_get_device_list(char **device_list_json);

esp_err_t matter_controller_control_device(char **result, uint64_t node_id, uint32_t cluster_id, uint32_t command_id,
                                           const char *command_params_json);

#ifdef __cplusplus
}
#endif