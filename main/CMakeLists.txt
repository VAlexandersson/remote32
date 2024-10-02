set(SOURCES main.c)

idf_component_register(SRCS ${SOURCES}
                    INCLUDE_DIRS "."
                    REQUIRES
                        freertos
                        esp_common
                        wifi_manager 
                        nvs_flash
                        )