idf_component_register( SRCS "gpio.cpp"
                        INCLUDE_DIRS "."
                        PRIV_REQUIRES flash dns error events ota gpio settings nvs_flash)