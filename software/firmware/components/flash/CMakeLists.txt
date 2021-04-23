idf_component_register( SRCS "flash.c" "filesystem.c"
                        INCLUDE_DIRS "."
                        PRIV_REQUIRES nvs_flash spiffs logging url error events netif json app_update
                        EMBED_FILES files/blacklist.html
                                    files/connected.html
                                    files/defaultblacklist.txt
                                    files/homepage.html
                                    files/scripts.js
                                    files/settings.html
                                    files/settings.json
                                    files/stylesheet.css
                                    files/wifi_select.html )