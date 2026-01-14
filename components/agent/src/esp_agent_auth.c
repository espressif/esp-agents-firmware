/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <esp_log.h>
#include <cJSON.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include <esp_crt_bundle.h>
#include <esp_http_client.h>

#include <esp_agent_auth.h>

#define CHUNK_SIZE 1024

static const char *TAG = "esp_agent_auth";
static const char *OAUTH_URL = "https://3pauth.rainmaker.espressif.com/oauth2/token";

esp_err_t esp_agent_auth_get_access_token(const char *refresh_token, char **access_token, size_t *access_token_len)
{
    if (!refresh_token || !access_token || !access_token_len) {
        ESP_LOGD(TAG, "Invalid parameters: refresh_token = %s, access_token = %s, access_token_len = %d", refresh_token, *access_token, *access_token_len);
        ESP_LOGE(TAG, "Invalid parameters");
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = ESP_OK;
    esp_http_client_handle_t client = NULL;
    cJSON *json = NULL;
    char *response_buffer = NULL;
    char *post_data = NULL;

    esp_http_client_config_t config = {
        .url = OAUTH_URL,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 10000,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return ESP_FAIL;
    }

    esp_http_client_set_header(client, "Content-Type", "application/x-www-form-urlencoded");

    size_t data_len = strlen("grant_type=refresh_token&client_id=1h7ujqjs8140n17v0ahb4n51m2&refresh_token=") + strlen(refresh_token) + 1;
    post_data = malloc(data_len);
    if (post_data == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for POST data");
        return ESP_ERR_NO_MEM;
    }
    snprintf(post_data, data_len, "grant_type=refresh_token&client_id=1h7ujqjs8140n17v0ahb4n51m2&refresh_token=%s", refresh_token);
    ESP_LOGD(TAG, "POST data (%d bytes)", strlen(post_data));
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    err = esp_http_client_open(client, strlen(post_data));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        goto end;
    }

    int wlen = esp_http_client_write(client, post_data, strlen(post_data));
    if (wlen < 0) {
        ESP_LOGE(TAG, "Failed to write POST data");
        err = ESP_FAIL;
        goto end;
    }
    ESP_LOGD(TAG, "Wrote %d bytes of POST data", wlen);

    // Fetch headers
    int content_length = esp_http_client_fetch_headers(client);
    int status_code = esp_http_client_get_status_code(client);
    bool is_chunked = esp_http_client_is_chunked_response(client);

    ESP_LOGI(TAG, "HTTP Status = %d, content_length = %d, chunked = %s",
             status_code, content_length, is_chunked ? "yes" : "no");

    if (status_code != 200) {
        ESP_LOGE(TAG, "HTTP request failed with status %d", status_code);
        err = ESP_ERR_INVALID_RESPONSE;
        goto end;
    }

    // Oauth server sends chunked response.

    size_t buffer_size = 2048;  // Start with 2KB buffer
    size_t total_read = 0;
    response_buffer = malloc(buffer_size);
    if (response_buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate initial response buffer");
        err = ESP_ERR_NO_MEM;
        goto end;
    }

    size_t chunk = 0;
    int data_read = 0;
    while ((data_read = esp_http_client_read(client, response_buffer + total_read, CHUNK_SIZE)) > 0) {
        ESP_LOGD(TAG, "Chunk %zu: read %d bytes", chunk, data_read);

        total_read += data_read;
        size_t buffer_remaining = buffer_size > total_read ? buffer_size - total_read : 0;
        // Check if we need to expand the buffer
        /* Always ensure we have at least 1.5 times CHUNK_SIZE of buffer remaining */
        if (buffer_remaining < (CHUNK_SIZE * 1.5)) {
            size_t new_size = buffer_size + (CHUNK_SIZE * 1.5);
            ESP_LOGD(TAG, "Expanding buffer from %zu to %zu bytes", buffer_size, new_size);
            char *new_buffer = realloc(response_buffer, new_size);
            if (new_buffer == NULL) {
                ESP_LOGE(TAG, "Failed to reallocate response buffer to %zu bytes", new_size);
                err = ESP_ERR_NO_MEM;
                goto end;
            }
            response_buffer = new_buffer;
            buffer_size = new_size;
        }
    }

    ESP_LOGI(TAG, "Finished reading response: %zu total bytes", total_read);

    if (total_read == 0) {
        ESP_LOGE(TAG, "No response data received");
        err = ESP_ERR_INVALID_RESPONSE;
        goto end;
    }

    response_buffer[total_read] = '\0';
    ESP_LOGI(TAG, "Response received (%zu bytes)", total_read);
    ESP_LOGD(TAG, "Response content: %s", response_buffer);

    // Parse JSON response
    json = cJSON_Parse(response_buffer);
    if (json == NULL) {
        ESP_LOGE(TAG, "Failed to parse JSON response");
        err = ESP_ERR_INVALID_RESPONSE;
        goto end;
    }

    // Extract access_token
    cJSON *access_token_json = cJSON_GetObjectItem(json, "access_token");
    if (access_token_json == NULL || !cJSON_IsString(access_token_json)) {
        ESP_LOGE(TAG, "access_token not found in response");
        err = ESP_ERR_INVALID_RESPONSE;
        goto end;
    }

    const char *token_value = cJSON_GetStringValue(access_token_json);
    if (token_value == NULL || strlen(token_value) == 0) {
        ESP_LOGE(TAG, "access_token is empty");
        err = ESP_ERR_INVALID_RESPONSE;
        goto end;
    }

    size_t token_len = strlen(token_value);
    *access_token = malloc(token_len + 1);
    if (*access_token == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for access token");
        err = ESP_ERR_NO_MEM;
        goto end;
    }

    strcpy(*access_token, token_value);
    *access_token_len = token_len;

    ESP_LOGI(TAG, "Successfully obtained access token (length: %d)", token_len);

end:
    // Cleanup
    if(json) {
        cJSON_Delete(json);
    }
    if(response_buffer) {
        free(response_buffer);
    }
    if(post_data) {
        free(post_data);
    }
    if(client) {
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
    }
    return err;
}
