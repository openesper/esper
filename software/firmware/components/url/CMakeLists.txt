idf_component_register(SRCS "url.c"
                    INCLUDE_DIRS "."
                    EMBED_FILES "default_blacklist.txt"
                    REQUIRES json
                    PRIV_REQUIRES spiffs error)