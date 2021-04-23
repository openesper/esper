idf_component_register( SRCS            "webserver.c"
                        INCLUDE_DIRS    "."
                        PRIV_REQUIRES   netif dns flash logging datetime url spiffs esp_http_server ota json app_update error events )