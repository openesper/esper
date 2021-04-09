idf_component_register( SRCS "dns.c"
                        INCLUDE_DIRS "."
                        PRIV_REQUIRES flash logging datetime gpio url netif error events json)