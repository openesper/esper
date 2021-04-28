idf_component_register(SRCS "url.c"
                    INCLUDE_DIRS "."
                    REQUIRES json
                    PRIV_REQUIRES spiffs error flash events)