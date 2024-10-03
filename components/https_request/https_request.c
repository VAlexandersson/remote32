
#include "https_request.h"
#include "esp_log.h"
#include "esp_tls.h"
#include "esp_http_client.h"

static const char *TAG = "HTTPS_REQUEST";

esp_err_t https_request_get(const char *url, char *response, size_t response_len) {
    esp_http_client_config_t config = {
        .url = url,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int content_length = esp_http_client_get_content_length(client);
        if (content_length < response_len) {
            esp_http_client_read(client, response, content_length);
            response[content_length] = '\0'; // Null-terminate the response
        } else {
            ESP_LOGE(TAG, "Response buffer too small");
            err = ESP_ERR_NO_MEM;
        }
    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    return err;
}

esp_err_t https_request_post(const char *url, const char *post_data, char *response, size_t response_len) {
    esp_http_client_config_t config = {
        .url = url,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int content_length = esp_http_client_get_content_length(client);
        if (content_length < response_len) {
            esp_http_client_read(client, response, content_length);
            response[content_length] = '\0'; // Null-terminate the response
        } else {
            ESP_LOGE(TAG, "Response buffer too small");
            err = ESP_ERR_NO_MEM;
        }
    } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    return err;
}