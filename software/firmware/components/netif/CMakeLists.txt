idf_component_register( SRCS "ip.cpp" "eth.cpp" "wifi.cpp"
                        INCLUDE_DIRS "include/"
                        PRIV_REQUIRES flash gpio error events json settings)