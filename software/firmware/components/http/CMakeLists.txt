idf_component_register( SRCS            "webserver.c"
                                        "application/home.c"
                                        "application/blacklist.c"
                                        "application/settings.c"
                                        "configuration/wifi_select.c"
                                        "configuration/connected.c"

                        INCLUDE_DIRS    "application/include/"
                                        "configuration/include/"
                                        "."
                    
                        PRIV_REQUIRES   netif dns flash logging datetime url spiffs esp_http_server ota json app_update error events

                        EMBED_FILES     "configuration/html/wifi_select.html"
                                        "configuration/html/connected.html"
                                        "application/html/home.html"
                                        "application/html/blacklist.html"
                                        "application/html/settings.html")