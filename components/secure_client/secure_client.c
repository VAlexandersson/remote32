#include "secure_client.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>
#define TAG "SECURE_CLIENT"
#define URI_BUFFER_SIZE 256

/*
esp_err_t secure_client_init(secure_client_t *client, const char *host, const char *port) {
    // init mbedTLS structures
    mbedtls_ssl_init(&client->ssl);
    mbedtls_x509_crt_init(&client->cacert);
    mbedtls_ctr_drbg_init(&client->ctr_drbg);
    mbedtls_ssl_config_init(&client->ssl_conf);
    mbedtls_entropy_init(&client->entropy);

    // Seed the RNG
    int ret = mbedtls_ctr_drbg_seed(&client->ctr_drbg, mbedtls_entropy_func, &client->entropy, NULL, 0);
    if (ret != 0) {
        ESP_LOGE(TAG, "mbedtls_ctr_drbg_seed failed: %d", ret);
        return ESP_FAIL;
    }


    // SSL/TLS configuration
    ret = mbedtls_ssl_config_defaults(&client->ssl_conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
    if (ret != 0) {
        ESP_LOGE(TAG, "mbedtls_ssl_config_defaults failed: %d", ret);
        return ESP_FAIL;
    }

    // Bypass certificate verification
    mbedtls_ssl_conf_authmode(&client->ssl_conf, MBEDTLS_SSL_VERIFY_NONE); 

    // Set RNG
    mbedtls_ssl_conf_rng(&client->ssl_conf, mbedtls_ctr_drbg_random, &client->ctr_drbg);

    return ESP_OK;
}
*/
    /*
    // Load crt bundle
    ret = esp_crt_bundle_attach(&client->ssl_conf);
    if (ret != 0) {
        ESP_LOGE(TAG, "esp_crt_bundle_attach failed: %d", ret);
        return ESP_FAIL;
    }
    */

esp_err_t secure_client_init(secure_client_t *client, const char *host, int port, const esp_tls_cfg_t *tls_cfg) {
    client->host = strdup(host);
    client->port = port;
    client->tls_cfg = tls_cfg;
    client->tls = NULL;
    return ESP_OK;
}

esp_err_t secure_client_connect(secure_client_t *client) {

    ESP_LOGI(TAG, "Connecting to %s:%d", client->host, client->port);

    // get client (char*)host and (int)port and concat into uri
    char uri[URI_BUFFER_SIZE];
    snprintf(uri, URI_BUFFER_SIZE, "%s:%d", client->host, client->port);

    // get len of host name:
    uint16_t host_len = strlen(client->host);

    client->tls = esp_tls_init();
    if (client->tls == NULL) {
        ESP_LOGE(TAG, "Failed to initialize TLS");
        return ESP_FAIL;
    }

    if (esp_tls_conn_new_sync(client->host, host_len, client->port, client->tls_cfg, client->tls) != 1) {
        ESP_LOGE(TAG, "Failed to connect to %s:%d", client->host, client->port);
        esp_tls_conn_destroy(client->tls);
        client->tls = NULL;
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Connected to %s:%d", client->host, client->port);

    return ESP_OK;
}

esp_err_t secure_client_write(secure_client_t *client, const char *data, uint16_t len) {
    if (client->tls == NULL) {
        ESP_LOGE(TAG, "TLS connection not established");
        return ESP_FAIL;
    }

    int ret = esp_tls_conn_write(client->tls, data, len);
    if (ret < 0) {
        ESP_LOGE(TAG, "Failed to write data: %d", ret);
        return ESP_FAIL;
    }

    return ESP_OK;
}

int secure_client_recv(secure_client_t *client, unsigned char *data, size_t len) {
    // ... (Receive data using mbedtls_ssl_read) ...
    return ESP_OK;
}

void secure_client_close(secure_client_t *client) {
    // ... (Close TLS connection and free resources) ...
}