/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <esp_err.h>
#include <esp_event.h>

#ifdef __cplusplus
extern "C" {
#endif

ESP_EVENT_DECLARE_BASE(AGENT_SETUP_EVENT);

typedef enum {
    /* This will be triggered when *all* of the following conditions are met:
     * 1. Network connectivity is established
     * 2. Agent ID is set
     * 3. Refresh token is set
     */
    AGENT_SETUP_EVENT_START,
    AGENT_SETUP_EVENT_NETWORK_CONNECTED,
    /**
     * It is not guaranteed that other conditions for starting agent are met when this event is triggered.
     * Agent should only be started on AGENT_SETUP_EVENT_START event.
     */
    AGENT_SETUP_EVENT_AGENT_ID_UPDATE,
} agent_setup_event_t;

/** @brief Initiaze the Agent Setup
 *
 * This will initialize the internal structures
 * and try to read the `agent_id` and `refresh_token` from NVS, if present.
 *
 * @return ESP_OK if sucess, error otherwise
 *
 * @note Even if the values are not available in NVS, this will still return success,
 * as it is a valid scenario for device factory reset.
 * @note Make sure it is called before network connectivity is established.
 */
esp_err_t agent_setup_init(void);

/**
 * @brief Start network provisioning
 *
 * This will start the network provisioning process. If the device has not been
 * provisioned yet, it will start the setup process.
 * If device is already provisioned, it will start the network and connect to the cloud.
 *
 * @return ESP_OK if success, error otherwise
 */
 esp_err_t agent_setup_start(void);

/**
 * @brief Get the locally stored agent_id
 *
 * @return Pointer to null terminated agent_id
 *
 * @note The output points to an internally allocated buffer.
 * caller should not free() the pointer
 */
char* agent_setup_get_agent_id(void);

/**
 * @brief Get the locally stored refresh_token
 *
 * @return Pointer to null terminated refresh_token
 *
 * @note The output points to an internally allocated buffer.
 * caller should not free() the pointer
 */
char* agent_setup_get_refresh_token(void);


/**
 * @brief Set the agent_id in internal buffer and in NVS
 *
 * @param agent_id Pointer to null terminated agent_id
 *
 * @note The agent_id is str-duped internally and the caller can free() the pointer after calling this function
 * @note This will result in AGENT_SETUP_EVENT_AGENT_ID_UPDATE event being triggered, even if network not connected and refresh token not set.
 *
 * @return ESP_OK if success, error otherwise
 */
esp_err_t agent_setup_set_agent_id(const char *agent_id);

/**
 * @brief Set the refresh_token
 *
 * @param refresh_token Pointer to null terminated refresh_token
 *
 * @note The refresh_token is str-duped internally and caller can free() the pointer after calling this function
 *
 * @return ESP_OK if success, error otherwise
 */
esp_err_t agent_setup_set_refresh_token(const char *refresh_token);

/**
 * @brief Factory reset the device
 *
 * @return ESP_OK if success, error otherwise
 */
esp_err_t agent_setup_factory_reset(void);

#ifdef __cplusplus
}
#endif
