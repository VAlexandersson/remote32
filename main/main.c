#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_event.h"
#include "esp_log.h"

#include "nvs_flash.h"

#include "wifi_manager.h"
#include "https_request.h"

#define LOG_LEVEL   ESP_LOG_VERBOSE
#define TAG "MAIN"

#define WIFI_SSID       CONFIG_WIFI_SSID
#define WIFI_PASSWORD   CONFIG_WIFI_PASSWORD


#include "lwip/sockets.h"
#include "lwip/netdb.h"

#define CHROMECAST_IP "10.0.0.11"
#define CHROMECAST_PORT 8009
#define CONNECT_MESSAGE "{\"type\": \"CONNECT\"}"
#define DISCONNECT_MESSAGE "{\"type\": \"CLOSE\"}"

#define TEST_URL "https://10.0.0.100:4443"
#define POST_DATA "{\"key\":\"TEST\"}"


static bool wifi_connected = false;

void wifi_status_callback(wifi_state_t status) {
    if (status == CONNECTED) {
        wifi_connected = true;
    } else {
        wifi_connected = false;
    }
}

void test_https_request_get() {
    char response[512]; // Adjust the size as needed
    esp_err_t err = https_request_get(TEST_URL, response, 512);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTPS GET request successful, response: %s", response);
    } else {
        ESP_LOGE(TAG, "HTTPS GET request failed: %s", esp_err_to_name(err));
    }
}

void test_https_request_post() {
    char response[1024]; // Adjust the size as needed
    esp_err_t err = https_request_post(TEST_URL, POST_DATA, response, 1024);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTPS POST request successful, response: %s", response);
    } else {
        ESP_LOGE(TAG, "HTTPS POST request failed: %s", esp_err_to_name(err));
    }
}


esp_err_t init_nvs(){
    esp_err_t status = nvs_flash_init();
    if (status == ESP_ERR_NVS_NO_FREE_PAGES || status == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        status = nvs_flash_erase();
        if (status == ESP_OK) {
            status = nvs_flash_init();
        }
    }
    return status;
}

void app_main() {    
    ESP_LOGI(TAG, "Creating default event loop");
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_LOGI(TAG, "Initializing NVS");
    ESP_ERROR_CHECK(nvs_flash_init());
    esp_err_t status = ESP_OK;

    status = wifi_init(WIFI_SSID, WIFI_PASSWORD);
    if (status != ESP_OK) {
        ESP_LOGE(TAG, "wifi_init failed: %s", esp_err_to_name(status));
        return;
    }

    register_status_callback(wifi_status_callback);
    while (!wifi_connected) {
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        ESP_LOGI(TAG, "Testing HTTPS POST request");
        test_https_request_post();
    
        ESP_LOGI(TAG, "Testing HTTPS GET request");
        test_https_request_get();
        vTaskDelay(60000 / portTICK_PERIOD_MS);
    }

}
