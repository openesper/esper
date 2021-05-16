![Esper](https://imgur.com/hzfZoeMl.jpg)

# Esper Software

## Installation

### Arduino

At the moment there is no support for the Arduino environment yet, but it is being worked on.

### Requirements

In order to build and flash Esper, you will first need to [install ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/#installation-step-by-step) then [set up the environment variables](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html#step-4-set-up-the-environment-variables). It is recommended to 
get one of the example "hello world" projects working before flashing Espers, just to verify that your toolchain is working.

After getting ESP-IDF setup, you will have to install the filesystem that Esper uses (LittleFS). This can be done with:

    cd esper
    git submodule init

### Configuration

Before building the Esper firmware, you will have to do some configuration first. If you are using a ESP32 development board, such as the ESP32 DevKitC, you will only have to enter
you wifi credentials. If you have other hardware, and you want to use ethernet or LEDs, then you will have to enable those features and then enter their respective GPIO numbers.

To edit the configuration, simply enter these commands:

    cd software/firmware
    idf.py menuconfig

Then configure the options under Esper Configuration to match your device's capabilities.

### Building & Flashing

After installing ESP-IDF, build and flash your device by running these commands (must be executed from firmware directroy)

    idf.py build
    idf.py -p PORT [-b BAUD] erase
    idf.py -p PORT [-b BAUD] flash monitor

### Issues

If you have any issues, [this section](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html#encountered-issues-while-flashing) may be able to help.