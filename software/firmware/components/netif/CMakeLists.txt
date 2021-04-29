idf_component_register( SRCS "ip.cpp" "eth.cpp" "wifi.cpp"
                        INCLUDE_DIRS "."
                        PRIV_REQUIRES flash gpio error events json)