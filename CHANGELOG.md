# Changelog

## 22.08 (August 12th, 2022)

### Features

- Updated Anjay to version 3.1.1
- Added CHANGELOG
- Added support for BG96
- Integrated with FreeRTOS Cellular Interface
- Added information about updating submodule to README

### Bugfixes

- Fixed partition offsets in m5stick-plus.bin binary file

## 22.06 (June 27th, 2022)

### Features

- Updated Anjay to version 3.0.0
- Added support for certificates and TCP socket
- Added non-secure connection option
- Added support for Send operation

## 22.04.1 (April 28th, 2022)

### Bugfixes

- Fix configuration issue regarded to improper include order.

## 22.04 (April 13th, 2022)

### Features

 - added support for FOTA
 - added support for changing Wi-Fi configuration while the device is
running
 - added NVS for Anjay and Wi-Fi configuration

### Improvements

 - all required configuration can be set from anjay-esp32-client component
config
 - updated ESP-IDF and Anjay

## 22.01.1 (January 26th, 2022)

### Bugfixes

 - Fixed configuration in avs_commons_config.h

## 22.01 (January 21st, 2022)

### Features

 - added support for M5StickC-Plus hardware
 - added generic sensors objects handling code

### Improvements

 - anjay handling WiFi connection loss

## 21.10 (October 4th, 2021)

### Features

- Initial release of Anjay-esp32-client
