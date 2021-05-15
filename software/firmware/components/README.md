# Directory of all components used in project

### Datetime

Used to get time from SNTP server

### DNS

All DNS related activities, including logging, are in here. This is where the socket, listening on port 53, is created and then parses any packets it receives.

### Error

This is where the ATTEMPT, TRY, and THROWE macros are, as well as most errors are defined. 

### Events

This component keeps track of the current state of the module using a FreeRTOS event group.

### Flash

Filesystem initialization and usage, as well as all of the files that will be embedded into the app binary

### GPIO

LED and button functions. LED task it notified any time the current state of the device is changed and updates accordingly.

### HTTP

This is the largest component. It stores all the code for both the application website.

### Lists

All functions that relate to the blacklist are in here.

### Lists

All functions that relate to the blacklist are in here.

### LittleFS

Submodule that contains the LittleFS filesystem, which is an alternative to SPIFFS.

### NetIF

Ethernet and Wifi code. Not used much after device is initialized and running.

### OTA

Over-The-Air updates. This contains the tasks that periodically check for updates and then writes the new update to flash.

### Settings

All functions related to parsing/editing the settings.json file

