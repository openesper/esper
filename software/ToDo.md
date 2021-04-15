## TODO:


### Tests:
- Start writing test doc
- Start incorporating unit testing
### Build
- Add Arduino support
### Web
- Password authentication
- Respond to POST requests with current results
- Analytics
    - Total queries and total blocked queries
    - most blocked/queried websites
- show currently connected network & ip
- show connected clients
- Add log page select to homepage 
- Upload/download blacklist files
- advanced settings fold
### Debugging
- Add debug page to website
- Add errors to error log
- remaining memory
    - total and for each task
### OTA Updates
- progress bar
### DATETIME
- Add timezone select
- Connect to SNTP in background, non-blocking
### CONFIGURATION
- Look into wifi_provisioning library
### DNS
- Support for non A/AAAA types
- better client handling
    - print error if client never got response
- Reverse DNS support
- Benchmark DNS response times
### LEDs
- LED Error Codes
### BLACKLIST
- more error handling errors
- Benchmark speed improvement from keeping blacklist in RAM instead of flash
### FLASH
- figure out way to emulate flash wear out & error handling
- Handle flash errors
- Document what is being held in flash

## Maybe Some Day:
- DHCP support
    - Individual block lists for devices
    - User accounts??
- DNS Over HTTPS (early tests dont look hopeful for ESP32, will likely need MPU)

## Known Bugs:
- Have to reset device if network ssid/pass change
- black/whitelist buttons on homepage do not update after being added 
