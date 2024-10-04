
#include "https_request.h"
#include "esp_log.h"
#include "esp_tls.h"
#include "esp_http_client.h"

#include <string.h>
#include <sys/param.h>
#include <stdlib.h>
#include <ctype.h>


#define TAG "HTTPS_REQUEST"
#define RESPONSE_BUFFER_SIZE 512
#define MAX_HTTP_OUTPUT_BUFFER 2048

extern const uint8_t server_cert_pem_start[] asm("_binary_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_cert_pem_end");
 

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    static char *output_buffer;  // Buffer to store the full response

    static int output_len;       // Stores number of bytes read
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGI(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);

            if (!esp_http_client_is_chunked_response(evt->client)) {
                // If user_data buffer is configured, copy the response into the buffer
                if (evt->user_data) {
                    if (output_len + evt->data_len < RESPONSE_BUFFER_SIZE) {
                        memcpy(evt->user_data + output_len, evt->data, evt->data_len);
                        output_len += evt->data_len;
                        char* user_data = (char*)evt->user_data;
                        user_data[output_len] = '\0'; // Null-terminate the buffer
                    }
                    else {
                        ESP_LOGE(TAG, "Response buffer too small");
                        return ESP_ERR_NO_MEM;
                    }
                }
            } else {
                ESP_LOGI(TAG, "Chunked response");
                if (output_buffer == NULL) { 
                    output_buffer = (char *) malloc(1); // Start with a small buffer
                    if (output_buffer == NULL) {
                        ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
                        return ESP_FAIL; 
                    }
                    output_len = 0;
                }
                output_buffer = realloc(output_buffer, output_len + evt->data_len + 1);
                if (output_buffer == NULL) {
                    ESP_LOGE(TAG, "Failed to reallocate memory for output buffer");
                    return ESP_FAIL;
                }
                memcpy(output_buffer + output_len, evt->data, evt->data_len);
                output_len += evt->data_len;
            }   
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
            if (output_buffer != NULL) {
                output_buffer[output_len] = '\0'; // Null-terminate the buffer
                if(evt->user_data) {
                    memcpy(evt->user_data, output_buffer, output_len);
                }
                free(output_buffer);
                output_buffer = NULL;
                output_len = 0;
            }
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            int mbedtls_err = 0;
            esp_err_t err = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t)evt->data, &mbedtls_err, NULL);
            if (err != 0) {
                ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
                ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
            }
            if (output_buffer != NULL) {
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGI(TAG, "HTTP_EVENT_REDIRECT");
            break;
    }
    return ESP_OK;
}


esp_err_t https_request_get(const char *url, char *response, size_t response_len) {

    esp_http_client_config_t config = {
        .url = url,
        .cert_pem = (const char*)server_cert_pem_start,
        .cert_len = server_cert_pem_end - server_cert_pem_start,
        .event_handler = _http_event_handler,
        .user_data = response,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %"PRId64,
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
    return err;
}


esp_err_t https_request_post(const char *url, const char *post_data, char *response, size_t response_len) {

    esp_http_client_config_t config = {
        .url = url,
        .cert_pem = (const char*)server_cert_pem_start,
        .cert_len = server_cert_pem_end - server_cert_pem_start,
        .event_handler = _http_event_handler,
        .user_data = response,
    };


    ESP_LOGI(TAG, "Connecting to URL: %s", url);
    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %"PRId64,
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
    return err;
}





/*
// function for reading response in chunks
esp_err_t read_chunked_response(esp_http_client_handle_t client, char *response, size_t response_len) {

    int total_read = 0;

    while(true) {
        
        size_t buffer_size_left = response_len - total_read;
        ESP_LOGI(TAG, "Buffer size left: %d", buffer_size_left);


        int read_len = esp_http_client_read(client, response + total_read, buffer_size_left);

        ESP_LOGI(TAG, "Read %d bytes", read_len);
        ESP_LOGI(TAG, "Response: %s", response);

        if (read_len < 0) {
            ESP_LOGE(TAG, "Failed to read response: %s", esp_err_to_name(read_len));
            return ESP_FAIL;
        } else if (read_len == 0) {
            response[total_read] = '\0';
            return ESP_OK;
        } else {
            total_read += read_len;
            if (total_read >= response_len) {
                ESP_LOGE(TAG, "Response buffer too small");
                return ESP_ERR_NO_MEM;
            }
        }
    }

}
    

esp_err_t read_response(esp_http_client_handle_t client, char *response, size_t response_len) {
    esp_err_t err = ESP_OK;
    int content_length = esp_http_client_get_content_length(client);
    ESP_LOGI(TAG, "Content length: %d", content_length);

    if (content_length == -1) {
        err = read_chunked_response(client, response, response_len);
    } else {
        if (content_length < response_len) {
            int read_len = esp_http_client_read(client, response, content_length);
            if (read_len >= 0) {
                response[content_length] = '\0'; // Null-terminate the response
            } else {
                ESP_LOGE(TAG, "Failed to read response: %s", esp_err_to_name(read_len));
                err = ESP_FAIL;
            }
        } else {
            ESP_LOGE(TAG, "Response buffer too small");
            err = ESP_ERR_NO_MEM;
        }
    }
    return err;
}
*/
/*
esp_err_t https_request_get(const char *url, char *response, size_t response_len) {


    ESP_LOGI(TAG, "Connecting to URL: %s", url);
    ESP_LOGI(TAG, "Response buffer size: %d", response_len);
    // ESP_LOGI(TAG, "Cert length: %d", config.cert_len);
    // ESP_LOGI(TAG, "Cert: %s", config.cert_pem);

    //set_config(url, response, response_len);
    default_config.url = url;

    esp_http_client_handle_t client = esp_http_client_init(&default_config);
//    esp_http_client_set_url(client, url);
    esp_http_client_set_method(client, HTTP_METHOD_GET);

    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP GET request successful");
        err = esp_http_client_get_status_code(client);

        ESP_LOGI(TAG, "Status code: %d", err);

        err = read_response(client, response, response_len);
    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }



    err = esp_http_client_cleanup(client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to clean up client: %s", esp_err_to_name(err));
    }
    return err;
}

esp_err_t https_request_post(const char *url, const char *post_data, char *response, size_t response_len) {

    esp_err_t err = ESP_OK;
    default_config.url = url;
    esp_http_client_handle_t client = esp_http_client_init(&default_config);


    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %"PRId64,
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }
    return err;

}
*/

    /*
    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_GET,
        // .cert_pem = (const char*)server_cert_pem_start,
        // .cert_len = server_cert_pem_end - server_cert_pem_start,
        .skip_cert_common_name_check = true,
        .use_global_ca_store = true
    };
    */


    /*
    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_POST,
        // .cert_pem = (const char*)server_cert_pem_start,
        // .cert_len = server_cert_pem_end - server_cert_pem_start,
        .skip_cert_common_name_check = true,
        .use_global_ca_store = true
    };
    */

/*

//    esp_http_client_set_url(client, url);
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    
    ESP_LOGI(TAG, "Connecting to URL: %s", url);

    // ESP_LOGI(TAG, "Cert length: %d", config.cert_len);
    // ESP_LOGI(TAG, "Cert: %s", config.cert_pem);

    esp_err_t err = esp_http_client_set_post_field(client, post_data, strlen(post_data));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set POST data: %s", esp_err_to_name(err));
        return err;
    }

    err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP Status = %d, content_length = %lld",
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }
    char buffer[1024];
    int data_read = esp_http_client_read(client, buffer, sizeof(buffer));
    if (data_read >= 0) {
        buffer[data_read] = 0; // Null-terminate the buffer
        ESP_LOGI(TAG, "Read %d bytes:\n%s", data_read, buffer);
    } else {
        ESP_LOGE(TAG, "Failed to read response");
    }

    esp_http_client_cleanup(client);
    return err;
*/