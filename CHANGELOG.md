# Changelog

## 24.11 (November 13th, 2024)

### Improvements
- Fixed compatibility with ESP IDF v5.3,
  support for ESP IDF in versions older than v5.0 is dropped

## 24.05 (May 28th, 2024)

### Improvements
- Increased main task stack size to support -Og optimization

## 24.02.1 (February 20th, 2024)

### Improvements
- Updated Anjay version

## 24.02 (February 8th, 2024)

### Improvements
- Cleaned up include headers

### Bugfixes
- Removed inappropriate mutexes in mpu6886 driver

## 23.11 (November 21st, 2023)

### Improvements
- Updated Anjay version

## 23.09 (September 7th, 2023)

### BREAKING CHANGES
- Removed Anjay and FreeRTOS-Cellular-Interface submodules and added Anjay-esp-idf
  component into the components/ directory

### Improvements
- Revamped configuration of Anjay and its dependencies
- The Anjay-esp-idf component includes a new `Component config/Anjay library
  configuration` menu for configuring the Anjay library
- Updated recommended ESP IDF to v4.4.5

## 22.12 (December 13th, 2022)

### Bugfixes

- Fixed a minor bug related to WiFi reconfiguration - after changing the Enable resource (12/x/1) value Anjay ESP32 Client didn't send the response code to the server

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
