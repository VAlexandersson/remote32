#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_event.h"
#include "esp_log.h"

#include "nvs_flash.h"

#include "wifi_manager.h"
#include "https_request.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
//#include "../libraries/cJSON/cJSON.h"
#include "cJSON.h"
#include "mdns.h"


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


void mdns_print_results(mdns_result_t *result) {
    mdns_result_t *current = result;
    while (current != NULL) {
        
        if (current->instance_name != NULL) {
            ESP_LOGI(TAG, "Hostname: %s", current->hostname);
        }

        if (current->addr != NULL) {
            ESP_LOGI(TAG, "IP address: %s", ip4addr_ntoa(&current->addr->addr));
        }
        if (current->service_type != NULL) {
            ESP_LOGI(TAG, "Service type: %s", current->service_type);
        }

        if(current->instance_name != NULL) {
            ESP_LOGI(TAG, "Instance name: %s", current->instance_name);
        }
        ESP_LOGI(TAG, "Port: %d", current->port);
        for (size_t i = 0; i < current->txt_count; i++) {
            if (current->txt[i].key && current->txt[i].value){
                ESP_LOGI(TAG, "TXT item: %s=%s", current->txt[i].key, current->txt[i].value);
            }
        }
        current = current->next;
    }
}

esp_err_t find_chromecast(mdns_result_t **result) {

    esp_err_t err = mdns_query_ptr("_googlecast", "_tcp", 5000, 1, result);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to query mDNS: %s", esp_err_to_name(err));
        return ESP_FAIL;
    }

    if (result == NULL) {
        ESP_LOGE(TAG, "No Chromecast found");
        return ESP_FAIL;
    }
    return ESP_OK;
}

void interact_with_chromecast(const char *ip_address) {
    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = inet_addr(ip_address);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(CHROMECAST_PORT);

    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        return;
    }

    ESP_LOGI(TAG, "Socket created, connecting to %s:%d", ip_address, CHROMECAST_PORT);
    int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
        ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
        close(sock);
        return;
    }

    ESP_LOGI(TAG, "Successfully connected");

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "PING");
    cJSON_AddStringToObject(root, "requestId", "1");
    const char *message = cJSON_Print(root);

    // Send the message
    int to_write = strlen(message);
    while (to_write > 0) {
        int written = send(sock, message, to_write, 0);
        if (written < 0) {
            ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
            break;
        }
        to_write -= written;
    }

    // Clean up
    cJSON_Delete(root);
    free((void *)message);

    // Receive response
    char rx_buffer[128];
    int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
    if (len < 0) {
        ESP_LOGE(TAG, "recv failed: errno %d", errno);
    } else {
        rx_buffer[len] = 0; // Null-terminate the response
        ESP_LOGI(TAG, "Received: %s", rx_buffer);
    }

    // Close the socket
    close(sock);
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
    ESP_LOGI(TAG, "WiFi connected");
    
    ESP_LOGI(TAG, "Initializing mDNS");
    mdns_init();
    mdns_hostname_set("esp32");
    mdns_instance_name_set("ESP32 Chromecast");

    ESP_LOGI(TAG, "FindinTrying to find Chromecast");
    mdns_result_t *result = NULL;
    status = find_chromecast(&result);
    ESP_LOGI(TAG, "Chromecast %s", status == ESP_OK ? "found" : "not found");
    if (status == ESP_OK) {
        mdns_print_results(result);
        interact_with_chromecast(ip4addr_ntoa(&result->addr->addr));
        mdns_query_results_free(result);
    }
   /*
    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        ESP_LOGI(TAG, "Testing HTTPS POST request");
        test_https_request_post();
    
        ESP_LOGI(TAG, "Testing HTTPS GET request");
        test_https_request_get();
        vTaskDelay(60000 / portTICK_PERIOD_MS);
    }
   */ 

}
