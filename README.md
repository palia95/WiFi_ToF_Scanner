# Wi-Fi FTM scanner and Serial reader

Authors [Emanuele Pagliari](https://github.com/Palia95)

This repo contains the source code for a Wi-Fi FTM scanner firmware for the ESP32S2 or ESP32S3 microcontroller, as well as a Python-based serial reader to collect the Wi-Fi scan result on the host machine.

## ESP32S2/3 firmware files

The 'ftm_scan' folder contains the source code to compile and flash a firmware for the ESP32S2 or ESP32S3 microcontroller, developed with the goal to scan the nearby Wi-Fi networks on a specific channel and gather the FTM info. The Wi-Fi scan results are then parsed into a JSON string and published to the host machine over the USB serial port.

The ESP-IDF 5.x.y is required for the compilation and flash. For more info regarding the ESP-IDF, check [here](https://docs.espressif.com/projects/esp-idf/en/v5.2.2/esp32s3/get-started/index.html)

## Python Serial reader

The 'ToF_RSSI_receiver' folder contains the source code for the Python serial reader. The JSON string publish by the ESP32S3 firmware are read on the specific port and saved into a CSV file.
