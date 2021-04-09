## Tests

### Initialization
1. LED is showing correct color during startup
2. Interfaces are initialized correctly, according to build options
3. Blocklists are successfully loaded in RAM
4. Logging file is opened
5. Webserver is initialized
6. LED is changed after initialization if finished

### Provisioning
1. Skip provisioning if wifi is disabled
2. Scan is started at boot
3. New scan is started when scan button is pressed
4. Error returned if ssid/pass is invalid
5. Connection successful if ssid/pass is valid
6. Ethernet saves IP if wifi is disabled and provisioning is skipped
7. Correct LED color is shown during provisioning

### Startup
1. Time is correctly received via SNTP 
2. Sockets created and listening on port 53
3. Webserver changes from provisioning to application correctly
4. Successfully checks for update at startup

### SNTP
1. Gets time from server and saves it to memory
2. Sets time to default value if SNTP server cant be reached
    - Continue to poll server if it fails
3. Changing timezone successfully updates current time

### DNS
1. Socket is created and listens for UDP packets on port 53
2. Packets larger than 512 bytes do not cause error
3. All DNS packets are forwarded if blocking is turned off
4. Packets that are not 'A' or 'AAAA' are forwarded
5. Responses from DNS server are successfully forward to correct client
6. DNS packets are 'blocked' if qname is in blacklist
7. DNS packets are 'captured' if provisioning is enabled or is qname matches device url

### GPIO
1. LEDs turn on and are correct colors
2. Pressing button toggles blocking state
3. Holding button for 5 seconds resets device
4. Holding button for 10 seconds rolls back to previous version

### Logging
1. DNS packets are logged successfully