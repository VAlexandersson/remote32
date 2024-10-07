#include <string.h>
#include <stdbool.h>

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
#include "secure_client.h"

#include "cast_channel.pb.h"

#include <pb.h>
#include <pb_encode.h>
#include <pb_decode.h>
#include <pb_common.h>

#define LOG_LEVEL   ESP_LOG_VERBOSE
#define TAG "MAIN"

#define WIFI_SSID       CONFIG_WIFI_SSID
#define WIFI_PASSWORD   CONFIG_WIFI_PASSWORD

const char CC_SOURCEID[] = "sender-0";
const char CC_MAIN_DESTIID[] = "receiver-0";

const char CC_NS_CONNECTION[] = "urn:x-cast:com.google.cast.tp.connection";
const char CC_NS_RECEIVER[] = "urn:x-cast:com.google.cast.receiver";
const char CC_NS_HEARTBEAT[] = "urn:x-cast:com.google.cast.tp.heartbeat";
const char CC_NS_MEDIA[] = "urn:x-cast:com.google.cast.media";

const char CC_MSG_CONNECT[] = "{\"type\": \"CONNECT\"}";
const char CC_MSG_PING[] = "{\"type\": \"PING\"}";
const char CC_MSG_GET_STATUS[] = "{\"type\": \"GET_STATUS\", \"requestId\": 1}"; 

const char CC_MSG_PLAY[] = "{\"type\": \"PLAY\", \"requestId\": 2, \"mediaSessionId\": ";
const char CC_MSG_PAUSE[] = "{\"type\": \"PAUSE\", \"requestId\": 2, \"mediaSessionId\": ";
const char CC_MSG_NEXT[] = "{\"type\": \"QUEUE_NEXT\", \"requestId\": 2, \"mediaSessionId\": ";
const char CC_MSG_PREV[] = "{\"type\": \"QUEUE_PREV\", \"requestId\": 2, \"mediaSessionId\": ";
const char CC_MSG_SET_VOL[] = "{\"type\": \"SET_VOLUME\", \"requestId\": 2, \"volume\": {\"level\": ";
const char CC_MSG_VOL_MUTE[] = "{\"type\": \"SET_VOLUME\", \"requestId\": 2, \"volume\": {\"muted\": ";
static char cc_msg_ctrl[128];

#include "lwip/sockets.h"
#include "lwip/netdb.h"

#define CHROMECAST_PORT 8009

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


void interact_with_chromecast(const char *ip_address, const uint16_t *port) {
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
    cJSON_AddStringToObject(root, "type", "GET_STATUS");
    cJSON_AddStringToObject(root, "requestId", "1");
    const char *message = cJSON_Print(root);
    ESP_LOGI(TAG, "Sending message: %s", message);
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
    ESP_LOGI(TAG, "Received %d bytes", len);
    if (len < 0) {
        ESP_LOGE(TAG, "recv failed: errno %d", errno);
    } else {
        rx_buffer[len] = 0; // Null-terminate the response
        ESP_LOGI(TAG, "Received: %s", rx_buffer);
    }

    // Close the socket
    close(sock);
}


#define BUFFERSIZE 5000 

bool encode_string(pb_ostream_t *stream, const pb_field_t *field, void * const *arg) {
    const char *str = (const char*)*arg;
    if (!pb_encode_tag_for_field(stream, field)) {
        return false;
    }
    return pb_encode_string(stream, (uint8_t*)str, strlen(str));
}

esp_err_t write_msg(secure_client_t *client, const char* nameSpace, const char* payload) {

    ESP_LOGI(TAG, "Writing message to Chromecast");
    uint8_t buffer[BUFFERSIZE];




    ESP_LOGI(TAG, "Populating message");
    extensions_api_cast_channel_CastMessage msg = extensions_api_cast_channel_CastMessage_init_zero;
    pb_ostream_t stream = pb_ostream_from_buffer(buffer+4, BUFFERSIZE-4);

    msg.protocol_version = extensions_api_cast_channel_CastMessage_ProtocolVersion_CASTV2_1_0;
    msg.source_id.arg = (void*)CC_SOURCEID;
    msg.source_id.funcs.encode = encode_string;
    msg.destination_id.arg = (void*)CC_MAIN_DESTIID;
    msg.destination_id.funcs.encode = encode_string;
    msg.namespace.arg = (void*)nameSpace;
    msg.namespace.funcs.encode = encode_string;
    msg.payload_type = extensions_api_cast_channel_CastMessage_PayloadType_STRING;
    msg.payload_utf8.arg = (void*)payload;
    msg.payload_utf8.funcs.encode = encode_string;
    
    ESP_LOGI(TAG, "Message populated: protocol_version=%d, source_id=%s, destination_id=%s, namespace=%s, payload_type=%d, payload_utf8=%s", msg.protocol_version, (char*)msg.source_id.arg, (char*)msg.destination_id.arg, (char*)msg.namespace.arg, msg.payload_type, (char*)msg.payload_utf8.arg);

    ESP_LOGI(TAG, "Source ID size: %d", strlen((char*)msg.source_id.arg));
    ESP_LOGI(TAG, "Destination ID size: %d", strlen((char*)msg.destination_id.arg));
    ESP_LOGI(TAG, "Namespace size: %d", strlen((char*)msg.namespace.arg));
    ESP_LOGI(TAG, "Payload size: %d", strlen((char*)msg.payload_utf8.arg));

    uint16_t encoded_size;

    if (!pb_get_encoded_size(&encoded_size, extensions_api_cast_channel_CastMessage_fields, &msg)) {
        ESP_LOGE(TAG, "Failed to get encoded size");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Encoded size: %d", encoded_size);

    if (encoded_size > BUFFERSIZE-4) {
        ESP_LOGE(TAG, "BUFFER TOO SMALL, required: %d, available %d", encoded_size, BUFFERSIZE-4);
        return ESP_FAIL;
    }


    ESP_LOGI(TAG, "Encoding message");
    if (!pb_encode(&stream, extensions_api_cast_channel_CastMessage_fields, &msg)) {
        ESP_LOGE(TAG, "Encoding failed: %s", PB_GET_ERROR(&stream));
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Encoded message size: %d", stream.bytes_written);


    buffer[0] = (uint8_t)(stream.bytes_written >> 24) & 0xFF;
    buffer[1] = (uint8_t)(stream.bytes_written >> 16) & 0xFF;
    buffer[2] = (uint8_t)(stream.bytes_written >> 8) & 0xFF;
    buffer[3] = (uint8_t)(stream.bytes_written >> 0) & 0xFF;


    int ret = esp_tls_conn_write(client->tls, buffer, stream.bytes_written+4);
    if (ret < 0) {
        ESP_LOGE(TAG, "Failed to write data: %d", ret);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Message sent successfully, size: %d", stream.bytes_written+4);

    return ESP_OK;
}

#define READ_BUFFER_SIZE 5000

esp_err_t read_msg(secure_client_t *client) {
    uint8_t buffer[READ_BUFFER_SIZE];
    int ret = esp_tls_conn_read(client->tls, buffer, READ_BUFFER_SIZE);

    if (ret < 0) {
        ESP_LOGE(TAG, "Failed to read data: %d, errno: %d", ret, errno);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Received message size: %d", ret);

    // Decode the message
    pb_istream_t stream = pb_istream_from_buffer(buffer + 4, ret - 4);
    extensions_api_cast_channel_CastMessage msg = extensions_api_cast_channel_CastMessage_init_zero;

    if (!pb_decode(&stream, extensions_api_cast_channel_CastMessage_fields, &msg)) {
        ESP_LOGE(TAG, "Decoding failed: %s", PB_GET_ERROR(&stream));
        return ESP_FAIL;
    }

    // ESP_LOGI(TAG, "Message populated: protocol_version=%d, source_id=%s, destination_id=%s, namespace=%s, payload_type=%d, payload_utf8=%s", msg.protocol_version, (char*)msg.source_id.arg, (char*)msg.destination_id.arg, (char*)msg.namespace.arg, msg.payload_type, (char*)msg.payload_utf8.arg);
    // Log the received message
    ESP_LOGI(TAG, "Received message: protocol_version=%d, source_id=%s, destination_id=%s, namespace=%s, payload_type=%d",
             msg.protocol_version,
             (char*)msg.source_id.arg,
             (char*)msg.destination_id.arg,
             (char*)msg.namespace.arg,
             msg.payload_type);

    if (msg.payload_type == extensions_api_cast_channel_CastMessage_PayloadType_STRING) {
        ESP_LOGI(TAG, "Payload: %s", (char*)msg.payload_utf8.arg);
    }

    return ESP_OK;
}




void mdns_print_results(mdns_result_t *result) {
    mdns_result_t *current = result;
    while (current != NULL) {
        ESP_LOGI(TAG, "PROTO: %s", current->proto);

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

extern const uint8_t chromecast_cert_pem_start[] asm("_binary_cert_pem_start");
extern const uint8_t chromecast_cert_pem_end[] asm("_binary_cert_pem_end");


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
    
    secure_client_t client;
    
    esp_tls_cfg_t tls_cfg = {
        .cacert_pem_buf  = chromecast_cert_pem_start,
        .cacert_pem_bytes = chromecast_cert_pem_end - chromecast_cert_pem_start,
    };        
    
    status = find_chromecast(&result);
    ESP_LOGI(TAG, "Chromecast %s", status == ESP_OK ? "found" : "not found");
    if (status == ESP_OK) {
        mdns_print_results(result);
        // interact_with_chromecast(ip4addr_ntoa(&result->addr->addr), &result->port);

        if (result && result->addr){
            secure_client_init(&client, ip4addr_ntoa(&result->addr->addr), result->port, &tls_cfg);
        } else {
            ESP_LOGE(TAG, "No IP address found for Chromecast");
        }
        mdns_query_results_free(result);
    }


    if (status == ESP_OK) {
        ESP_LOGI(TAG, "Connecting to Chromecast: %s:%d", client.host, client.port);
        uint16_t host_len = strlen(client.host);

        client.tls = esp_tls_init();
        if (client.tls == NULL) {
            ESP_LOGE(TAG, "Failed to initialize TLS");
            status = ESP_FAIL;
        }
        if (status == ESP_OK) {
            if (esp_tls_conn_new_sync(client.host, host_len, client.port, client.tls_cfg, client.tls) != 1) {
                ESP_LOGE(TAG, "Failed to connect to %s:%d", client.host, client.port);
                esp_tls_conn_destroy(client.tls);
                client.tls = NULL;
                status =  ESP_FAIL;
            }
        }
    }

    if (status == ESP_OK) {
        ESP_LOGI(TAG, "Connected to %s:%d", client.host, client.port);
        status = write_msg(&client, CC_NS_CONNECTION, CC_MSG_CONNECT);
        ESP_LOGI(TAG, "Message succesfully sent: %s", status == ESP_OK ? "YES" : "NO");
        
        status = write_msg(&client, CC_NS_HEARTBEAT, CC_MSG_PING);

        if (status == ESP_OK) {
            // Wait for response
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            
            // Read and log the response
            read_msg(&client);
        } else {
            ESP_LOGE(TAG, "Failed to send message PING");
        }


    }

    free(client.host);
    esp_tls_conn_destroy(client.tls);


}
