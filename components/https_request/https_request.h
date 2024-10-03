#ifndef HTTPS_REQUEST_H
#define HTTPS_REQUEST_H

#include "esp_err.h"

// Function to initialize the HTTPS component
esp_err_t https_request_init(void);

// Function to send an HTTPS GET request
esp_err_t https_request_get(const char *url, char *response, size_t response_len);

// Function to send an HTTPS POST request
esp_err_t https_request_post(const char *url, const char *post_data, char *response, size_t response_len);

#endif // HTTPS_REQUEST_H