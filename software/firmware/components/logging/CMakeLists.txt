idf_component_register( SRCS "logging.c"
                        INCLUDE_DIRS "."
                        REQUIRES json url
                        PRIV_REQUIRES dns flash datetime spiffs esp_http_server error)