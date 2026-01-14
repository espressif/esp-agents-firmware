/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <agent_setup.h>
#include <agent_console.h>
#include <setup/console.h>

#include <esp_check.h>

static const char *TAG = "setup_console";

esp_err_t console_refresh_token_cmd_handler(int argc, char **argv)
{
    if (argc != 2) {
        ESP_LOGE(TAG, "Usage: set-token <refresh_token>");
        return ESP_ERR_INVALID_ARG;
    }
    const char *token = argv[1];

    ESP_LOGD(TAG, "Received Refresh Token: %s", token);
    agent_setup_set_refresh_token(token);

    return ESP_OK;
}

esp_err_t console_agent_id_cmd_handler(int argc, char **argv)
{
    if (argc != 2) {
        ESP_LOGE(TAG, "Usage: set-agent <agent_id>");
        return ESP_ERR_INVALID_ARG;
    }
    const char *agent_id = argv[1];

    ESP_LOGD(TAG, "Received Agent ID: %s", agent_id);
    agent_setup_set_agent_id(agent_id);

    return ESP_OK;
}

esp_err_t setup_console_register_commands()
{
    esp_console_cmd_t cmds[] = {
        {
        .command = "set-token",
        .help = "Set the refresh token\nUsage: set-token <refresh_token>",
        .func = console_refresh_token_cmd_handler,
        },
        {
            .command = "set-agent",
            .help = "Set the agent id\nUsage: set-agent <agent_id>",
            .func = console_agent_id_cmd_handler,
        },
    };
    size_t cmds_len = sizeof(cmds) / sizeof(cmds[0]);

    for (int i=0; i<cmds_len; i++){
        ESP_RETURN_ON_ERROR(agent_console_register_command(&cmds[i]), TAG, "Failed to register console command: %s", cmds[i].command);
    }
    return ESP_OK;

}
