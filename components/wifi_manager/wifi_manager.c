#include "wifi_manager.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include <string.h>

static const char *TAG = "WIFI_MANAGER";
static wifi_state_t current_status = NOT_INITIALIZED;
static wifi_event_callback_t event_callback = NULL;

wifi_state_t wifi_get_status(void) {
    return current_status;
}

esp_err_t register_status_callback(wifi_event_callback_t callback) {
    if (callback == NULL) {
        ESP_LOGE(TAG, "callback is NULL");
        return ESP_ERR_INVALID_ARG;
    }
    event_callback = callback;
    return ESP_OK;
}

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    esp_err_t status = ESP_OK;
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
        case WIFI_EVENT_STA_START:
            ESP_LOGI(TAG, "STA_START");

            status = esp_wifi_connect();
            if (status == ESP_OK) {
                current_status = CONNECTING;
                ESP_LOGI(TAG, "CONNECTING");
            }
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            ESP_LOGI(TAG, "STA_DISCONNECTED");
            current_status = DISCONNECTED;
            ESP_LOGI(TAG, "WiFi disconnected");
            break;
        case WIFI_EVENT_STA_CONNECTED:
            ESP_LOGI(TAG, "STA_CONNECTED");
            current_status = WAITING_FOR_IP;
            ESP_LOGI(TAG, "Waiting for IP address...");
            break;
        default:
            ESP_LOGW(TAG, "Unhandled WIFI event: %ld", event_id);
            break;
        }
    }
    else if (event_base == IP_EVENT) {
        switch (event_id) {
        case IP_EVENT_STA_GOT_IP:
            ESP_LOGI(TAG, "STA_GOT_IP");
            current_status = CONNECTED;
            ESP_LOGI(TAG, "CONNECTED, got IP address");
            break;
        case IP_EVENT_STA_LOST_IP:
            ESP_LOGI(TAG, "STA_LOST_IP");
            current_status = WAITING_FOR_IP;
            ESP_LOGI(TAG, "WAITING_FOR_IP, Lost IP address");
            break;
        default:
            ESP_LOGW(TAG, "Unhandled IP event: %ld", event_id);
            break;
        }
    }
    if (event_callback != NULL) {
        event_callback(current_status);
    }
}

esp_err_t wifi_init(const char *ssid, const char *password) {

    if (ssid == NULL || password == NULL) {
        ESP_LOGE(TAG, "ssid or password is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    if (current_status != NOT_INITIALIZED) {
        if (current_status == ERROR) {
            ESP_LOGE(TAG, "WiFi is in ERROR state");
            current_status = NOT_INITIALIZED;
        } else ESP_LOGE(TAG, "WiFi already initialized");
        return ESP_OK;
    }

    esp_err_t status = esp_netif_init();
    if (status != ESP_OK) {
        ESP_LOGE(TAG, "esp_netif_init failed: %s", esp_err_to_name(status));
        return status;
    }

    if (!esp_netif_create_default_wifi_sta()) {
        ESP_LOGE(TAG, "esp_netif_create_default_wifi_sta failed");
        return ESP_FAIL;
    }

    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    status = esp_wifi_init(&wifi_init_config);
    if (status != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_init failed: %s", esp_err_to_name(status));
        return status;
    }

    status = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL);
    if (status != ESP_OK) {
        ESP_LOGE(TAG, "esp_event_handler_register for WIFI_EVENT failed: %s", esp_err_to_name(status));
        return status;
    }

    status = esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL);
    if (status != ESP_OK) {
        ESP_LOGE(TAG, "esp_event_handler_register for IP_EVENT failed: %s", esp_err_to_name(status));
        return status;
    }

    status = esp_wifi_set_mode(WIFI_MODE_STA);
    if (status != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_set_mode failed: %s", esp_err_to_name(status));
        return status;
    }

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "",
            .password = "",
        },
    };

    strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));

    status = esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
    if (status != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_set_config failed: %s", esp_err_to_name(status));
        return status;
    }

    status = esp_wifi_start();
    if (status != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_start failed: %s", esp_err_to_name(status));
        return status;
    }

    current_status = INITIALIZED;

    return ESP_OK;
}
