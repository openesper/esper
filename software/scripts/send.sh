#!/bin/bash

scp `dirname "$0"`/../firmware/build/*.bin `dirname "$0"`/../firmware/build/bootloader/bootloader.bin `dirname "$0"`/../firmware/build/ota_data_initial.bin `dirname "$0"`/../firmware/build/partition_table/partition-table.bin $1;
