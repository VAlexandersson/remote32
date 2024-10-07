set(SOURCES secure_client.c)

idf_component_register(SRCS ${SOURCES}
                    INCLUDE_DIRS "."
                    REQUIRES 
                        esp-tls 
                        esp_http_client
                        mbedtls
                        esp_netif
                    )