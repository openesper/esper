idf_component_register( SRCS "ip.c" "eth.c" "wifi.c"
                        INCLUDE_DIRS "."
                        PRIV_REQUIRES flash gpio error events json)