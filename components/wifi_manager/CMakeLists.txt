set(SOURCES wifi_manager.c)

idf_component_register(SRCS ${SOURCES}
                    INCLUDE_DIRS "."
                    REQUIRES esp_wifi)