#!/bin/bash
cd website
hugo
cd ..
python  `dirname "$0"`/compress.py
python  `dirname "$0"`/reformat_blacklist.py
idf.py build -C  `dirname "$0"`/../firmware/
