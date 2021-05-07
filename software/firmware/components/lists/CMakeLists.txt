idf_component_register(SRCS "lists.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES json
                    PRIV_REQUIRES spiffs error flash events)