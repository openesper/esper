idf_component_register( SRCS "ip.cpp" "eth.cpp" "wifi.cpp"
                        INCLUDE_DIRS "include/"
                        PRIV_REQUIRES error events settings flash nvs_flash)