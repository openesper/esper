idf_component_register(SRCS "url.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES json
                    PRIV_REQUIRES spiffs error flash events)