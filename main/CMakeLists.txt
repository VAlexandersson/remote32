set(SOURCES main.c)

idf_component_register(SRCS ${SOURCES}
                    INCLUDE_DIRS "."
                    REQUIRES
                        freertos
                        esp_common
                        nvs_flash
                        wifi_manager 
                        https_request
                        cJSON
                        secure_client
                        nanopb
                        protobuf
                    EMBED_FILES 
                        "../certs/cert.pem")