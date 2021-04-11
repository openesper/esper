#!/bin/bash
python  `dirname "$0"`/compress.py
python  `dirname "$0"`/reformat_blacklist.py
cd website
hugo
cd ..
idf.py build -C  `dirname "$0"`/../firmware/
