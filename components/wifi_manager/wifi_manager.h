#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_err.h"
#include "esp_wifi_types.h"

#include "esp_mac.h"

typedef enum {
    NOT_INITIALIZED,
    INITIALIZED,
    READY_TO_CONNECT,
    CONNECTING,
    CONNECTED,
    WAITING_FOR_IP,
    DISCONNECTED,
    ERROR
} wifi_state_t;

typedef void (*wifi_event_callback_t)(wifi_state_t state);

esp_err_t wifi_init(const char *ssid, const char *password);
esp_err_t register_wifi_event_callback(wifi_event_callback_t callback);
wifi_state_t wifi_get_status(void);

#endif // WIFI_MANAGER_H
