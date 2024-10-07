// secure_client.h
#ifndef SECURE_CLIENT_H
#define SECURE_CLIENT_H

#include "esp_err.h"
#include "mbedtls/ssl.h" 
#include "mbedtls/net_sockets.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/error.h"
#include "esp_crt_bundle.h"

#include <esp_tls.h>

typedef struct {
    esp_tls_cfg_t *tls_cfg;
    esp_tls_t *tls;
    char *host;
    int port;
} secure_client_t;

esp_err_t secure_client_init(secure_client_t *client, const char *host, int port, const esp_tls_cfg_t *tls_cfg);
esp_err_t secure_client_connect(secure_client_t *client);
esp_err_t secure_client_write(secure_client_t *client, const char *data, uint16_t len);
esp_err_t secure_client_read(secure_client_t *client, char *data, int len);
void secure_client_disconnect(secure_client_t *client);


/*
typedef struct {
    mbedtls_ssl_context ssl;
    mbedtls_net_context server_fd;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_x509_crt cacert;
    mbedtls_ssl_config ssl_conf;
    char *host;
    char *port;
} secure_client_t;
*/

//esp_err_t secure_client_init(secure_client_t *client, const char *host, const char *port);
// esp_err_t secure_client_connect(secure_client_t *client);
int secure_client_send(secure_client_t *client, const unsigned char *data, size_t len);
int secure_client_recv(secure_client_t *client, unsigned char *data, size_t len);
void secure_client_close(secure_client_t *client);

#endif