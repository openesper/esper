idf_component_register( SRCS "ota.cpp"
                        INCLUDE_DIRS "."
                        PRIV_REQUIRES error events gpio flash nvs_flash app_update esp_http_client esp_https_ota json settings)