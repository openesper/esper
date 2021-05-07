idf_component_register( SRCS            "webserver.cpp" "post_handlers.cpp" "get_handlers.cpp" "put_handlers.cpp" "delete_handlers.cpp"
                        INCLUDE_DIRS    "include/"
                        PRIV_REQUIRES   netif dns flash datetime lists spiffs esp_http_server ota json app_update error events settings)