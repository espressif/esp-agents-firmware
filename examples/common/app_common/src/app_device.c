#include <esp_check.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <esp_timer.h>

#include <agent_setup.h>
#include <esp_board_device.h>
#include <driver/gpio.h>

#include "app_agent.h"
#include "app_audio.h"
#include "app_device.h"
#include "app_capacitive_touch.h"
#include "app_touch_press.h"
#include "board_defs.h"
#ifdef INDICATOR_DEVICE_NAME
#include <dev_gpio_ctrl.h>
#endif

static const char *TAG = "app_device";

#define REMINDER_DISPLAY_TIMEOUT_SECONDS 5
#define AGENT_SLEEP_TIMEOUT_SECONDS 15

typedef enum {
    DEVICE_STATE_IDLE,
    DEVICE_STATE_LISTENING,
    DEVICE_STATE_SPEAKING,
    DEVICE_STATE_MAX,
} device_state_t;

typedef enum {
    DEVICE_ACTION_MICROPHONE_START,
    DEVICE_ACTION_MICROPHONE_PAUSE,
    DEVICE_ACTION_MICROPHONE_STOP,
    DEVICE_ACTION_SPEAKER_START,
    DEVICE_ACTION_SPEAKER_STOP,
    DEVICE_ACTION_SLEEP_TIMER_START,
    DEVICE_ACTION_SLEEP_TIMER_STOP,
    DEVICE_ACTION_MAX,
} device_action_t;

typedef struct {
    app_device_event_t event;
    device_event_data_t data;
    bool has_data;
    bool system_initialized;
} device_event_container_t;

typedef struct {
    device_state_t state;
    QueueHandle_t message_queue;
    bool init_done;
    bool wakeup;
    bool wakeup_start_pending;
    esp_timer_handle_t sleep_timer;
    bool reminder_active;
    esp_timer_handle_t reminder_complete_timer;
    bool system_initialized;
    esp_err_t (*set_text_cb)(app_device_text_type_t text_type, const char *text, void *priv_data);
    esp_err_t (*system_state_changed_cb)(app_device_system_state_t new_state, void *priv_data);
    void *priv_data;
} device_data_t;

device_data_t g_device_data;

extern const uint8_t wakeup_mp3_start[] asm("_binary_wakeup_mp3_start");
extern const uint8_t wakeup_mp3_end[] asm("_binary_wakeup_mp3_end");

extern const uint8_t finish_reminder_mp3_start[] asm("_binary_finish_reminder_mp3_start");
extern const uint8_t finish_reminder_mp3_end[] asm("_binary_finish_reminder_mp3_end");

static void device_sleep_timer_callback(void *arg)
{
    /* AFE doesn't emit wakeup_end event when manually triggered */
    app_device_event_enqueue(DEVICE_EVENT_SLEEP);
    app_audio_trigger_sleep();
}

static void reminder_complete_timer_callback(void *arg)
{
    app_device_event_enqueue(DEVICE_EVENT_REMINDER_COMPLETE);
}

static void device_set_text(app_device_text_type_t type, const char *text)
{
    if (!text) {
        return;
    }

    if (g_device_data.set_text_cb) {
        g_device_data.set_text_cb(type, text, g_device_data.priv_data);
    }
}

static void device_notify_state_changed(app_device_system_state_t state)
{
    if (g_device_data.system_state_changed_cb) {
        g_device_data.system_state_changed_cb(state, g_device_data.priv_data);
    }
}

static const char *deivce_get_agent_state_text(void)
{
    /* Don't update if reminder is active */
    if (g_device_data.reminder_active) {
        return NULL;
    }

    app_agent_state_t agent_state = app_agent_get_state();

    switch (agent_state) {
        case APP_AGENT_STATE_CONNECTING:
        case APP_AGENT_STATE_CONNECTED:
            return "Starting...";
            break;
        case APP_AGENT_STATE_DISCONNECTED:
            return "Standby";
            break;
        case APP_AGENT_STATE_STARTED:
            if (g_device_data.state == DEVICE_STATE_IDLE) {
                return "Zzzz...";
            }
            break;
        default:
            return NULL;
            break;
    }
    return NULL;
}

static void device_update_led(bool listening)
{
#ifdef INDICATOR_DEVICE_NAME
    periph_gpio_handle_t *gpio_handle;
    ESP_RETURN_VOID_ON_ERROR(esp_board_device_get_handle(INDICATOR_DEVICE_NAME, (void **)&gpio_handle), TAG, "Failed to get GPIO handle");

    bool led_on = listening;
    gpio_set_level(gpio_handle->gpio_num, led_on ? 0 : 1);
#endif
}

static void device_perform_action(device_action_t action)
{
    switch (action) {
        case DEVICE_ACTION_MICROPHONE_START:
            app_audio_microphone_set_state(MICROPHONE_STATE_START);
            device_update_led(true);
            break;

        case DEVICE_ACTION_MICROPHONE_STOP:
            app_audio_microphone_set_state(MICROPHONE_STATE_STOP);
            device_update_led(false);
            break;

        case DEVICE_ACTION_MICROPHONE_PAUSE:
            app_audio_microphone_set_state(MICROPHONE_STATE_PAUSE);
            device_update_led(false);
            break;

        case DEVICE_ACTION_SPEAKER_START:
            app_audio_speaker_start();
            break;

        case DEVICE_ACTION_SPEAKER_STOP:
            app_audio_speaker_stop();
            break;

        case DEVICE_ACTION_SLEEP_TIMER_STOP:
            esp_timer_stop(g_device_data.sleep_timer);
            break;

        case DEVICE_ACTION_SLEEP_TIMER_START:
            esp_timer_start_once(g_device_data.sleep_timer, AGENT_SLEEP_TIMEOUT_SECONDS * 1000000ULL);
            break;

        default:
            ESP_LOGW(TAG, "Action not handled: %d", action);
            break;
    }

}

void device_process_event(app_device_event_t event, void *data)
{
    device_event_container_t *container = (device_event_container_t *)data;
    device_event_data_t event_data;
    bool has_data = false;

    if (container) {
        event_data = container->data;
        has_data = container->has_data;
    }

    switch (event) {
        case DEVICE_EVENT_SYSTEM_INITIALIZED:
            g_device_data.system_initialized = true;
            break;

        case DEVICE_EVENT_SPEECH_START:
            /* Send dummy audio data */
            if (g_device_data.state == DEVICE_STATE_IDLE) {
                break;
            }

            device_perform_action(DEVICE_ACTION_MICROPHONE_PAUSE);
            device_perform_action(DEVICE_ACTION_SPEAKER_START);
            device_perform_action(DEVICE_ACTION_SLEEP_TIMER_STOP);

            g_device_data.state = DEVICE_STATE_SPEAKING;
            break;

        case DEVICE_EVENT_SPEECH_END:
            if (g_device_data.state == DEVICE_STATE_SPEAKING) {
                app_audio_speaker_download_complete();
            }
            break;

        case DEVICE_EVENT_SPEECH_PLAYBACK_COMPLETE:
            if (g_device_data.reminder_active) {
                esp_timer_start_once(g_device_data.reminder_complete_timer, REMINDER_DISPLAY_TIMEOUT_SECONDS * 1000000ULL);
                app_audio_play_media_async("embed://audio/0_reminder.mp3", finish_reminder_mp3_start, finish_reminder_mp3_end - finish_reminder_mp3_start);
            }
            /* Wakeup event will take care of stopping speaker and turning on microphone*/
            app_device_event_enqueue(DEVICE_EVENT_WAKEUP);
            break;

        case DEVICE_EVENT_WAKEUP:
            if (!g_device_data.system_initialized) {
                break;
            }

            if (!app_agent_is_active()) {
                g_device_data.wakeup_start_pending = true;
                app_agent_connect();
                break;
            }

            if (g_device_data.state == DEVICE_STATE_IDLE){
                app_audio_play_media_async("embed://audio/0_wakeup.mp3", wakeup_mp3_start, wakeup_mp3_end - wakeup_mp3_start);
                app_agent_speech_conversation_start();
            }

            device_perform_action(DEVICE_ACTION_SPEAKER_STOP);
            device_perform_action(DEVICE_ACTION_MICROPHONE_START);

            /* Restart sleep timer */
            device_perform_action(DEVICE_ACTION_SLEEP_TIMER_STOP);
            device_perform_action(DEVICE_ACTION_SLEEP_TIMER_START);

            g_device_data.state = DEVICE_STATE_LISTENING;
            device_notify_state_changed(APP_DEVICE_SYSTEM_STATE_LISTENING);
            break;

        case DEVICE_EVENT_SLEEP:
            device_set_text(APP_DEVICE_TEXT_TYPE_SYSTEM, "Zzzz...");
            device_perform_action(DEVICE_ACTION_MICROPHONE_STOP);
            device_perform_action(DEVICE_ACTION_SLEEP_TIMER_STOP);

            app_agent_speech_conversation_end();

            device_notify_state_changed(APP_DEVICE_SYSTEM_STATE_SLEEP);
            g_device_data.state = DEVICE_STATE_IDLE;
            break;

        case DEVICE_EVENT_INTERRUPT:
            if (g_device_data.state == DEVICE_STATE_LISTENING) {
                app_device_event_enqueue(DEVICE_EVENT_SLEEP);
            } else if (g_device_data.state == DEVICE_STATE_SPEAKING) {
                device_perform_action(DEVICE_ACTION_SPEAKER_STOP);
                app_device_event_enqueue(DEVICE_EVENT_SPEECH_END);
            } else {
                /* Will wait for current playback to complete and then start the microphone again */
                app_device_event_enqueue(DEVICE_EVENT_WAKEUP);
            }
            break;

        case DEVICE_EVENT_FACTORY_RESET:
            device_set_text(APP_DEVICE_TEXT_TYPE_SYSTEM, "Release to factory reset");
            break;

        case DEVICE_EVENT_AGENT_STATE_CHANGED:
            // Update display text based on agent state
            if (!g_device_data.reminder_active) {
                device_set_text(APP_DEVICE_TEXT_TYPE_SYSTEM, deivce_get_agent_state_text());
            }
            if (app_agent_get_state() == APP_AGENT_STATE_STARTED && g_device_data.wakeup_start_pending) {
                app_device_event_enqueue(DEVICE_EVENT_WAKEUP);
                g_device_data.wakeup_start_pending = false;
            }
            break;

        case DEVICE_EVENT_REMINDER:
            {
                // Get reminder text from event data
                if (!has_data || !event_data.text) {
                    ESP_LOGE(TAG, "Reminder event without text");
                    break;
                }

                const char *reminder_text = event_data.text;

                // Mark reminder as active
                g_device_data.reminder_active = true;

                // Display reminder text immediately
                char display_text[256];
                snprintf(display_text, sizeof(display_text), "Reminder: %s", reminder_text);
                device_set_text(APP_DEVICE_TEXT_TYPE_SYSTEM, display_text);

                // Free the reminder text after copying (it was allocated in set_reminder_tool_cb)
                free((char *)reminder_text);

                /* Play the reminder chime instantly if device is not speaking */
                if (g_device_data.state != DEVICE_STATE_SPEAKING) {
                    app_audio_play_media_async("embed://audio/0_reminder.mp3", finish_reminder_mp3_start, finish_reminder_mp3_end - finish_reminder_mp3_start);
                    esp_timer_start_once(g_device_data.reminder_complete_timer, REMINDER_DISPLAY_TIMEOUT_SECONDS * 1000000ULL);
                }

                /* Play the reminder chime after audio playback is complete, keep the system text visible */
            }
            break;

        case DEVICE_EVENT_REMINDER_COMPLETE:
            g_device_data.reminder_active = false;
            device_set_text(APP_DEVICE_TEXT_TYPE_SYSTEM, deivce_get_agent_state_text());
            break;

        case DEVICE_EVENT_SET_USER_TEXT:
            if (has_data && event_data.text && g_device_data.state != DEVICE_STATE_IDLE) {
                device_set_text(APP_DEVICE_TEXT_TYPE_USER, event_data.text);
                free((char *)event_data.text);
            }
            break;

        case DEVICE_EVENT_SET_ASSISTANT_TEXT:
            if (has_data && event_data.text && g_device_data.state != DEVICE_STATE_IDLE) {
                device_set_text(APP_DEVICE_TEXT_TYPE_ASSISTANT, event_data.text);
                free((char *)event_data.text);
            }
            break;

        default:
            ESP_LOGW(TAG, "Event not handled: %d", event);
            break;
    }
}

void device_process_task(void *arg)
{
    // Wait for message in queue
    device_event_container_t message;
    while (true) {
        xQueueReceive(g_device_data.message_queue, &message, portMAX_DELAY);
        device_process_event(message.event, &message);
    }
}

esp_err_t app_device_event_enqueue(app_device_event_t event)
{
    device_event_container_t message;
    message.event = event;
    message.has_data = false;
    return xQueueSend(g_device_data.message_queue, &message, portMAX_DELAY);
}

esp_err_t app_device_event_enqueue_with_data(app_device_event_t event, device_event_data_t *data)
{
    device_event_container_t message;
    message.event = event;
    if (data) {
        message.data = *data;
        message.has_data = true;
    } else {
        message.has_data = false;
    }
    return xQueueSend(g_device_data.message_queue, &message, portMAX_DELAY);
}

esp_err_t app_device_init(app_device_config_t *config)
{
    if (g_device_data.init_done) {
        return ESP_OK;
    }

    g_device_data.set_text_cb = config->set_text_cb;
    g_device_data.system_state_changed_cb = config->system_state_changed_cb;
    g_device_data.priv_data = config->priv_data;

    g_device_data.state = DEVICE_STATE_IDLE;
    g_device_data.message_queue = xQueueCreate(10, sizeof(device_event_container_t));
    if (g_device_data.message_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create message queue");
        return ESP_ERR_NO_MEM;
    }

    if (xTaskCreate(device_process_task, "device_process_task", 4096, NULL, 5, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create device process task");
        return ESP_ERR_NO_MEM;
    }

    esp_timer_create_args_t sleep_timer_args = {
        .callback = device_sleep_timer_callback,
        .arg = NULL,
        .name = "device_sleep_timer",
    };
    ESP_RETURN_ON_ERROR(esp_timer_create(&sleep_timer_args, &g_device_data.sleep_timer), TAG, "Failed to create device sleep timer");

    // Initialize reminder complete timer
    esp_timer_create_args_t reminder_timer_args = {
        .callback = reminder_complete_timer_callback,
        .arg = NULL,
        .name = "reminder_complete_timer",
    };
    ESP_RETURN_ON_ERROR(esp_timer_create(&reminder_timer_args, &g_device_data.reminder_complete_timer), TAG, "Failed to create reminder complete timer");

    // Initialize reminder state
    g_device_data.reminder_active = false;

    g_device_data.init_done = true;
    device_update_led(false);

    ESP_RETURN_ON_ERROR(app_touch_press_init(), TAG, "Failed to initialize touch press");
    ESP_RETURN_ON_ERROR(app_capacitive_touch_init(), TAG, "Failed to initialize touch sensor");

    return ESP_OK;
}
