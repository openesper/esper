import os
import sys

script_dir, script_path = os.path.split(os.path.abspath(sys.argv[0]))
outputFile = script_dir + r'/../firmware/components/url/default_blacklist.txt'
inputFile = script_dir + r'/../defaultBlacklist/blacklist.txt'

with open(inputFile, 'rb') as input:
    with open(outputFile, 'wb') as output:
        for url in input.readlines():
            url = url.strip(b'\r\n')
            url_length = len(url)
            length_byte = bytes([url_length])
            output.write(length_byte+url)

print("Reformated Blacklist file.")
