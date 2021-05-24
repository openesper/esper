idf_component_register( SRCS "dns.cpp" "server.cpp" "logging.cpp"
                        INCLUDE_DIRS "include/"
                        PRIV_REQUIRES error events settings datetime lists)