# Anjay ESP32 Client
## Overview
This repository contains a LwM2M Client application example for ESP32 devices, based on open-source [Anjay](https://github.com/AVSystem/Anjay) library and [Espressif IoT Development Framework](https://github.com/espressif/esp-idf).

The following boards are supported natively in the project:
- ESP-WROVER-KIT
- ESP32-DevKitC
- M5StickC-Plus

You may also use other boards, by selecting the `Unknown` board and manually configuring available peripherals through Kconfig.

The following LwM2M Objects are supported:
| Target         | Objects
|----------------|---------------------------------------------
| Common         | Security (/0)<br>Server (/1)<br>Device (/3)<br>Firmware Update (/5)<br>WLAN connectivity (/12)
| ESP-WROVER-KIT | Push button (/3347)<br>Light control (/3311)
| ESP32-DevKitC  | Push button (/3347)
| M5StickC-Plus  | Push button (/3347)<br>Light control (/3311)<br>Temperature sensor (/3303)<br>Accelerometer (/3313)<br>Gyroscope (/3343)

## Compiling and launching
1. Install ESP-IDF and its dependencies on your computer. Please follow the instructions at https://docs.espressif.com/projects/esp-idf/en/v4.4/esp32/get-started/index.html up to and including the point where you call `. $HOME/esp/esp-idf/export.sh`
   * The project has been tested with ESP-IDF v4.4, but may work with other versions as well.
2. Run `idf.py set-target esp32` in the project directory
3. Run `idf.py menuconfig`
   * navigate to `Component config/anjay-esp32-client`:
     * select one of supported boards or manually configure the board in `Board options` menu
     * configure Anjay in `Client options` menu
     * configure WiFi in `Connection configuration` menu
4. Run `idf.py build` to compile
5. Run `idf.py flash` to flash
   * NOTE: M5StickC-Plus does not support default baudrate, run `idf.py -b 750000 flash` to flash it
6. The logs will be on the same `/dev/ttyUSB<n>` port that the above used for flashing, 115200 8N1
   * You can use `idf.py monitor` to see logs on serial output from a connected device, or even more conveniently `idf.py flash monitor` as one command to see logs right after the device is flashed

## Connecting to the LwM2M Server
To connect to [Coiote IoT Device Management](https://www.avsystem.com/products/coiote-iot-device-management-platform/) LwM2M Server, please register at [https://www.avsystem.com/try-anjay/](https://www.avsystem.com/try-anjay/). The default Server URI (Kconfig option `ANJAY_CLIENT_SERVER_URI`) is set to try-anjay server, but you must manually set other client configuration options.

NOTE: You may use any LwM2M Server compliant with LwM2M 1.0 TS. The server URI
can be changed in the example configuration options.

## Connecting using NVS provided credentials
It is possible to use pre-built binaries to flash the board and provide credentials by flashing a NVS partition binary.
To do that, `esptool.py` is required, which can be installed running `pip install esptool`

### Creating a merged binary for M5StickC-Plus
```
esptool.py --chip esp32  merge_bin --flash_mode dio --flash_size 2MB --flash_freq 40m 0x1000 build/bootloader/bootloader.bin 0x8000 build/partition_table/partition-table.bin 0x10000 build/anjay-esp32-client.bin 0x110000 build/storage.bin --output m5stickc-plus.bin
```
### NVS config file
To generate NVS partition, create a `nvs_config.csv` file with following content:
```
key,type,encoding,value
config,namespace,,
wifi_ssid,data,string,[wifi_ssid]
wifi_pswd,data,string,[wifi_password]
wifi_inter_en,data,u8,1
identity,data,string,[identity]
psk,data,string,[psk]
uri,data,string,[lwm2m_server_uri]
writable_wifi,namespace,,
wifi_ssid,data,string,[wifi_ssid]
wifi_pswd,data,string,[wifi_password]
wifi_inter_en,data,u8,0
```
And fill proper values for `[wifi_ssid]`, `[wifi_password]`, `[identity]`, `[psk]`, `[lwm2m_server_uri]`.
After that create config partition by running:
```
python3 $IDF_PATH/components/nvs_flash/nvs_partition_generator/nvs_partition_gen.py generate nvs_config.csv nvs_config.bin 0x4000
```
### Flashing the firmware and partitions
After partition is created, flash firmware and configuration using following commands (baudrate may vary depending on your board):
```
esptool.py -b 750000 --chip esp32 write_flash 0x0000 m5stickc-plus.bin
esptool.py -b 750000 --chip esp32 write_flash 0x9000 nvs_config.bin
```
Device will be reset and run with provided configuration.
## Links
* [Anjay source repository](https://github.com/AVSystem/Anjay)
* [Anjay documentation](https://avsystem.github.io/Anjay-doc/index.html)
* [Doxygen-generated API documentation](https://avsystem.github.io/Anjay-doc/api/index.html)
* [AVSystem IoT Devzone](https://iotdevzone.avsystem.com/)
* [AVSystem Discord server](https://discord.avsystem.com)
