/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <esp_log.h>
#include <esp_check.h>

#include <esp_rmaker_common_console.h>

#include <agent_console.h>

static const char *TAG = "agent_console";

static esp_console_repl_t *g_repl = NULL;

void start_console_task(void *arg)
{
    esp_console_start_repl(g_repl);
    vTaskDelete(NULL);
}

esp_err_t agent_console_init(void)
{
    if (g_repl) {
        ESP_LOGE(TAG, "Console REPL already initialized");
        return ESP_ERR_INVALID_STATE;
    }

    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.max_cmdline_length = 3072; // 2KB required for refresh token, in worst case

#if defined(CONFIG_ESP_CONSOLE_UART_DEFAULT) || defined(CONFIG_ESP_CONSOLE_UART_CUSTOM)
    esp_console_dev_uart_config_t hw_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_console_new_repl_uart(&hw_config, &repl_config, &repl), TAG, "Failed to create console REPL");
#elif defined(CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG)
    esp_console_dev_usb_serial_jtag_config_t hw_config = ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_console_new_repl_usb_serial_jtag(&hw_config, &repl_config, &g_repl), TAG, "Failed to create console REPL");
#endif

    /* Start esp console from dedicated task so main task is not blocked when console couldn't be started
     * (Power without USB)
     */
    xTaskCreate(start_console_task, "start_console_task", 4096, NULL, 1, NULL);

    /* Register default commands */
    ESP_RETURN_ON_ERROR(agent_console_register_default_commands(), TAG, "Failed to register default commands");

    return ESP_OK;
}

esp_err_t agent_console_register_default_commands(void)
{
    if (!g_repl) {
        ESP_LOGE(TAG, "Console REPL not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    esp_rmaker_common_register_commands();
    return ESP_OK;
}

esp_err_t agent_console_register_command(const esp_console_cmd_t *cmd)
{
    if (!g_repl) {
        ESP_LOGE(TAG, "Console REPL not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    return esp_console_cmd_register(cmd);
}
