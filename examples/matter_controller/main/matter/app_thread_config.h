/**
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <esp_err.h>
#include <esp_openthread_types.h>
#include <esp_rcp_update.h>
#include <sdkconfig.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ESP_OPENTHREAD_DEFAULT_RADIO_CONFIG()                                  \
  {                                                                            \
    .radio_mode = RADIO_MODE_UART_RCP,                                         \
    .radio_uart_config = {                                                     \
        .port = UART_NUM_1,                                                    \
        .uart_config =                                                         \
            {                                                                  \
                .baud_rate = 460800,                                           \
                .data_bits = UART_DATA_8_BITS,                                 \
                .parity = UART_PARITY_DISABLE,                                 \
                .stop_bits = UART_STOP_BITS_1,                                 \
                .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,                         \
                .rx_flow_ctrl_thresh = 0,                                      \
                .source_clk = UART_SCLK_DEFAULT,                               \
            },                                                                 \
        .rx_pin = GPIO_NUM_10,                                                 \
        .tx_pin = GPIO_NUM_17,                                                 \
    },                                                                         \
  }

#define ESP_OPENTHREAD_DEFAULT_HOST_CONFIG()                                   \
  {                                                                            \
    .host_connection_mode = HOST_CONNECTION_MODE_NONE,                      \
    .host_usb_config = USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT(),                \
  }

#define ESP_OPENTHREAD_DEFAULT_PORT_CONFIG()                                   \
  {                                                                            \
    .storage_partition_name = "nvs", .netif_queue_size = 10,                   \
    .task_queue_size = 10,                                                     \
  }

#ifdef CONFIG_AUTO_UPDATE_RCP
#define ESP_OPENTHREAD_RCP_UPDATE_CONFIG()                                                                           \
    {                                                                                                                \
        .rcp_type = RCP_TYPE_ESP32H2_UART, .uart_rx_pin = 10, .uart_tx_pin = 17,                                     \
        .uart_port = 1, .uart_baudrate = 115200, .reset_pin = 7,                                                     \
        .boot_pin = 18, .update_baudrate = 460800,                                                                    \
        .firmware_dir = "/" CONFIG_RCP_PARTITION_NAME "/ot_rcp", .target_chip = ESP32H2_CHIP,                        \
    }
#endif

#ifdef __cplusplus
}
#endif
