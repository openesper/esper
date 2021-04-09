idf_component_register( SRCS "gpio.c"
                        INCLUDE_DIRS "."
                        PRIV_REQUIRES flash dns error events ota)