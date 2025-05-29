# Anjay ESP32 Client

## ⚠️ Archived Repository

**This repository is no longer actively maintained by the team. It is provided as is for reference purposes only.**
**We do not accept issues, pull requests, or provide support.**
**If you are still interested in this project or have questions, feel free to contact us. https://avsystem.com/contact/**

---

## Overview
This repository contains a LwM2M Client application example for ESP32 devices, based on open-source [Anjay-esp-idf](https://github.com/AVSystem/Anjay-esp-idf) component and [Espressif IoT Development Framework](https://github.com/espressif/esp-idf).

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
1. Install ESP-IDF and its dependencies on your computer. Please follow the instructions at https://docs.espressif.com/projects/esp-idf/en/v5.3.1/esp32/get-started/index.html including `Manual Installation` up to the `Start a Project` subtitle.
   * The project has been tested with ESP-IDF v5.3.1, but may work with other v5.x.x versions as well. Older versions are not supported.
1. Clone the repository `git clone https://github.com/AVSystem/Anjay-esp32-client.git` and navigate to project directory
1. Initialize and update submodules with `git submodule update --init --recursive`
1. Run `idf.py set-target esp32` in the project directory
1. Run `idf.py menuconfig`
   * navigate to `Component config/anjay-esp32-client`:
     * select one of supported boards or manually configure the board in `Board options` menu
     * configure LwM2M client configuration in `Client options` menu
     * configure Wi-Fi in `Connection configuration` menu
   * navigate to `Component config/Anjay library configuration` to configure `Anjay`
     library and its dependencies (`avs_commons` and `avs_coap`)
1. Run `idf.py build` to compile
1. Run `idf.py flash` to flash
   * NOTE: M5StickC-Plus does not support default baudrate, run `idf.py -b 750000 flash` to flash it
1. The logs will be on the same `/dev/ttyUSB<n>` port that the above used for flashing, 115200 8N1
   * You can use `idf.py monitor` to see logs on serial output from a connected device, or even more conveniently `idf.py flash monitor` as one command to see logs right after the device is flashed

## Connecting to the LwM2M Server
To connect to [Coiote IoT Device Management](https://www.avsystem.com/products/coiote-iot-device-management-platform/) LwM2M Server, please register at [https://eu.iot.avsystem.cloud/](https://eu.iot.avsystem.cloud/). The default Server URI (Kconfig option `ANJAY_CLIENT_SERVER_URI`) is set to EU Cloud Coiote DM instance, but you must manually set other client configuration options.

NOTE: You may use any LwM2M Server compliant with LwM2M 1.0 TS. The server URI
can be changed in the example configuration options.

## Connecting using NVS provided credentials
It is possible to use pre-built binaries to flash the board and provide credentials by flashing a NVS partition binary.
To do that, `esptool.py` is required, which can be installed running `pip install esptool`

### Creating a merged binary for M5StickC-Plus
```
esptool.py --chip esp32  merge_bin --flash_mode dio --flash_size 4MB --flash_freq 40m 0x1000 build/bootloader/bootloader.bin 0x8000 build/partition_table/partition-table.bin 0x10000 build/anjay-esp32-client.bin 0x310000 build/storage.bin --output m5stickc-plus.bin
```
### NVS config file
To generate NVS partition, create a `nvs_config.csv` file with following content:
```
key,type,encoding,value
config,namespace,,
wifi_ssid,data,string,[wifi_ssid]
wifi_pswd,data,string,[wifi_password]
wifi_inter_en,data,u8,1
endpoint_name,data,string,[endpoint_name]
identity,data,string,[identity]
psk,data,string,[psk]
uri,data,string,[lwm2m_server_uri]
writable_wifi,namespace,,
wifi_ssid,data,string,[wifi_ssid]
wifi_pswd,data,string,[wifi_password]
wifi_inter_en,data,u8,0
```
And fill proper values for `[wifi_ssid]`, `[wifi_password]`, `[endpoint_name]`, `[identity]`, `[psk]`, `[lwm2m_server_uri]`.
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
### TCP socket
To switch to TCP socket instead of UDP run `idf.py menuconfig`, navigate to `Component config/anjay-esp32-client/Client options/Choose socket` and select TCP (remember that you must also provide a proper URI in the `nvs_config.csv` file, e.g. `coaps+tcp://eu.iot.avsystem.cloud:5684`).
### ESP32 with certificates
1. Prepare your certificates. All certificates should have a `.der` extension and should be added to the directory where this `README.md` file is located. The names of the certificates should be as follows:
   * client public certificate - `client_cert.der`
   * client private certificate - `client_key.der`
   * server public certificate - `server_cert.der`
1. Run `idf.py menuconfig`, navigate to `Component config/anjay-esp32-client/Client options/Choose security mode` and select `Certificates`.
### FOTA
After compilation, you can perform FOTA with Coiote DM. Required binary file location:
```
$PROJECT_DIR/build/anjay-esp32-client/build/anjay-esp32-client.bin
```
### ESP32 with BG96 and FreeRTOS cellular library
1. Prepare your BG96 module, connect it to the selected ESP32 UART interface.
1. Run `idf.py menuconfig`
   * navigate to `Component config/anjay-esp32-client`:
      * select `External BG96 module` in `Choose an interface` menu
   * navigate to `Component config/Anjay library configuration/Enable support for external BG96 module`:
      * configure BG96 in `BG96 module configuration` menu
      * configure PDN authentication type in `PDN authentication type` menu
      * configure the `APN name` (and `PDN username/password` if needed)

## Links
* [Anjay source repository](https://github.com/AVSystem/Anjay)
* [Anjay documentation](https://avsystem.github.io/Anjay-doc/index.html)
* [Doxygen-generated API documentation](https://avsystem.github.io/Anjay-doc/api/index.html)
* [AVSystem IoT Devzone](https://iotdevzone.avsystem.com/)
* [AVSystem Discord server](https://discord.avsystem.com)
* [FreeRTOS cellular library](https://www.freertos.org/cellular/index.html)
