/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 #pragma once

#include <esp_err.h>

/** @brief Get Oauth access token from RainMaker Refresh Token
 *
 * @param[in] refresh_token A null terminated string pointint to refresh token
 * @param[out] access_token Memory location to store the newly allocated access token
 * @param[out] access_token_len The length of access token
 *
 * @return ESP_OK if success, error otherwise.
 */
esp_err_t esp_agent_auth_get_access_token(const char *refresh_token, char **access_token, size_t *access_token_len);
