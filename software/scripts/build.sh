#!/bin/bash
cd website
hugo
cd ..
python  `dirname "$0"`/compress.py
python  `dirname "$0"`/reformat_blacklist.py
python  $IDF_PATH/components/nvs_flash/nvs_partition_generator/nvs_partition_gen.py generate \
        `dirname "$0"`/../firmware/configuration/configuration.csv \
        `dirname "$0"`/../firmware/configuration/conf.bin 0x4000
idf.py build -C  `dirname "$0"`/../firmware/
