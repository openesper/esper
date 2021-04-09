idf_component_register( SRCS "flash.c"
                        INCLUDE_DIRS "."
                        PRIV_REQUIRES nvs_flash spiffs logging url error netif json)