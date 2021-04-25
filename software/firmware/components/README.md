# Directory of all components used in project

### Datetime

Used to get time from SNTP server

### DNS

All DNS related activities are in here. This is where the socket, listening on port 53, is created and then parses any packets it receives.

### Error

This is where the ATTEMPT macro, as well as most errors are defined. 

### Events

This component keeps track of the current state of the module using a FreeRTOS event group. 
Any component get set/clear/check any of the event bits at any time.

### Flash

All methods that retreive data from flash are located in this component. It stores blacklist, settings, and logging data to be retreived by other components.

### GPIO

LED and button functions. LED task it notified any time the current state of the device is changed and updates accordingly.

### HTTP

This is the largest component. It stores all the code for both the application and configuration websites.

### Logging

The logging task is in here. The task is notified any time a DNS query is made and then adds it's data to the log file.

### NetIF

Ethernet and Wifi code. Not used much after device is initialized and running.

### OTA

Over-The-Air updates. This contains the tasks that periodically check for updates and then writes the new update to flash.

### URL

Slightly confusing name, this is where the blacklist is kept. All functions that relate to the blacklist as well as url parsing are in here.