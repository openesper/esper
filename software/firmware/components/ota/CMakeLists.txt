idf_component_register( SRCS "ota.c"
                        INCLUDE_DIRS "."
                        PRIV_REQUIRES error events gpio url flash nvs_flash app_update esp_http_client esp_https_ota json)