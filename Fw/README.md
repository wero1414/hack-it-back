# Firmware 

This firmware is intended to use the Arduino Nano 33 BLE as core using the arduino board support based on the MBED ecosystem.

Please install the MBED core for the Arduino Nano 33 BLE you can follow this guide https://www.arduino.cc/en/Guide/NANO33BLE

## Central 

Firmware for the main controller for fan speed

Dependencies: 

 - https://github.com/arduino-libraries/ArduinoBLE


 ## Node

 Firmware for the near sensor to feed the main controller data. 

 Dependencies:

 - https://github.com/arduino-libraries/ArduinoBLE
 - https://github.com/Sensirion/arduino-i2c-scd4x
 - https://github.com/Sensirion/arduino-i2c-sgp40

 