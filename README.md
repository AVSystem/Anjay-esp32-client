# Anjay ESP32 Client
## Overview
This repository contains a LwM2M Client application example for ESP32 devices, based on open-source [Anjay](https://github.com/AVSystem/Anjay) library and [Espressif IoT Development Framework](https://github.com/espressif/esp-idf).

The following boards are supported natively in the project:
- ESP-WROVER-KIT
- ESP32-DevKitC

You may also other boards, by selecting the `Unknown` board and manually configuring available peripherals through Kconfig.

The following LwM2M Objects are supported:
| Target         | Objects
|----------------|---------------------------------------------
| Common         | Security (/0)<br>Server (/1)<br>Device (/3)
| ESP-WROVER-KIT | Push button (/3347)<br>Light control (/3311)
| ESP32-DevKitC  | Push button (/3347)

## Compiling and launching
1. Install ESP-IDF and its dependencies on your computer. Please follow the instructions at https://docs.espressif.com/projects/esp-idf/en/v4.3/esp32/get-started/index.html up to and including the point where you call `. $HOME/esp/esp-idf/export.sh`
   * The project has been tested with ESP-IDF v4.3, but may work with other versions as well.
2. Run `idf.py set-target esp32` in the project directory
3. Run `idf.py menuconfig`
   * navigate to `Example Connection Configuration` and fill in the WiFi settings (or switch to Ethernet if using that)
   * navigate to `Component config/anjay-esp32-client`:
     * select one of supported boards or manually configure the board in `Board options` menu
     * configure Anjay in `Client options` menu
4. Run `idf.py build` to compile
5. Run `idf.py flash` to flash
6. The logs will be on the same `/dev/ttyUSB<n>` port that the above used for flashing, 115200 8N1

## Connecting to the LwM2M Server
To connect to [Coiote IoT Device Management](https://www.avsystem.com/products/coiote-iot-device-management-platform/) LwM2M Server, please register at [https://www.avsystem.com/try-anjay/](https://www.avsystem.com/try-anjay/). The default Server URI (Kconfig option `ANJAY_CLIENT_SERVER_URI`) is set to try-anjay server, but you must manually set other client configuration options.

NOTE: You may use any LwM2M Server compliant with LwM2M 1.0 TS. The server URI
can be changed in the example configuration options.
