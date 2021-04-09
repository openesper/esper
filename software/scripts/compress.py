import os
import sys
from shutil import copyfile

script_dir, script_path = os.path.split(os.path.abspath(sys.argv[0]))
output_dir = script_dir + r'/../firmware/components/http/'
input_dir = script_dir + r'/../website/public'

files = [
    (input_dir+r'/application/blacklist/index.html',        output_dir+r'/application/html/blacklist.html'      ),
    (input_dir+r'/application/home/index.html',             output_dir+r'/application/html/home.html'           ),
    (input_dir+r'/application/settings/index.html',         output_dir+r'/application/html/settings.html'       ),
    (input_dir+r'/configuration/connected/index.html',      output_dir+r'/configuration/html/connected.html'    ),
    (input_dir+r'/configuration/wifi_select/index.html',    output_dir+r'/configuration/html/wifi_select.html'  ),
]

print('Creating output directory...')
if not os.path.exists(output_dir):
    os.mkdir(output_dir)
    os.mkdir(output_dir+r'/application')
    os.mkdir(output_dir+r'/configuration')

for file in files:
    with open(file[0], 'r') as src:
        with open(file[1], 'w') as dest:
            print("Writing to %s" % file[1])
            lines = src.readlines()
            for line in lines:
                dest.write(line.strip(' \t\r'))

print('Compressed HTML')