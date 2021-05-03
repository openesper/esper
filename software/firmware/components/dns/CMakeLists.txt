idf_component_register( SRCS "dns.cpp"
                        INCLUDE_DIRS "."
                        PRIV_REQUIRES logging url netif error events settings)