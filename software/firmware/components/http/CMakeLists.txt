idf_component_register( SRCS            "webserver.c" "post_handlers.c" "get_handlers.c" "put_handlers.c" "delete_handlers.c"
                        INCLUDE_DIRS    "include/"
                        PRIV_REQUIRES   netif dns flash logging datetime url spiffs esp_http_server ota json app_update error events )