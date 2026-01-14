/**
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * This file defines a state machine for the device.
 * It is responsible for the state transitions like wakeup->listening->speaking->sleep.
 * It also handles playing the reminder chime and displaying the reminder text.
 */

#pragma once

#include <esp_err.h>

typedef enum {
    DEVICE_EVENT_SYSTEM_INITIALIZED,
    DEVICE_EVENT_SPEECH_START,
    DEVICE_EVENT_SPEECH_END,
    DEVICE_EVENT_SPEECH_PLAYBACK_COMPLETE,
    DEVICE_EVENT_WAKEUP,
    DEVICE_EVENT_SLEEP,
    DEVICE_EVENT_INTERRUPT,
    DEVICE_EVENT_FACTORY_RESET,
    DEVICE_EVENT_AGENT_STATE_CHANGED,
    DEVICE_EVENT_REMINDER,
    DEVICE_EVENT_REMINDER_COMPLETE,
    DEVICE_EVENT_SET_USER_TEXT,
    DEVICE_EVENT_SET_ASSISTANT_TEXT,
    DEVICE_EVENT_MAX,
} app_device_event_t;

// Event data union for different event types
typedef union {
    const char *text;           // For REMINDER, SET_USER_TEXT, SET_ASSISTANT_TEXT events
} device_event_data_t;

typedef enum {
    APP_DEVICE_TEXT_TYPE_USER,
    APP_DEVICE_TEXT_TYPE_ASSISTANT,
    APP_DEVICE_TEXT_TYPE_SYSTEM,
} app_device_text_type_t;

typedef enum {
    APP_DEVICE_SYSTEM_STATE_SLEEP,
    APP_DEVICE_SYSTEM_STATE_ACTIVE,
    APP_DEVICE_SYSTEM_STATE_LISTENING,
} app_device_system_state_t;

typedef struct {
    esp_err_t (*set_text_cb)(app_device_text_type_t text_type, const char *text, void *priv_data);
    esp_err_t (*system_state_changed_cb)(app_device_system_state_t new_state, void *priv_data);
    void *priv_data;
} app_device_config_t;

/* Event enqueue functions */
esp_err_t app_device_event_enqueue(app_device_event_t event);
esp_err_t app_device_event_enqueue_with_data(app_device_event_t event, device_event_data_t *data);
esp_err_t app_device_init(app_device_config_t *config);
