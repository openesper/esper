idf_component_register( SRCS "flash.cpp" "filesystem.cpp"
                        INCLUDE_DIRS "."
                        PRIV_REQUIRES nvs_flash spiffs logging url error events netif json app_update littlefs settings
                        EMBED_FILES files/blacklist.html
                                    files/connected.html
                                    files/defaultblacklist.txt
                                    files/homepage.html
                                    files/app_scripts.js
                                    files/settings.html
                                    files/defaultsettings.json
                                    files/stylesheet.css
                                    files/prov_scripts.js
                                    files/wifi_select.html 
                                    files/404.html )