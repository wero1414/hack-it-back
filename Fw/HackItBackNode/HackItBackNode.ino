#include <Arduino.h>
#include <ArduinoBLE.h>
#include <SensirionI2CSgp40.h>
#include <SensirionI2CScd4x.h>
#include <Wire.h>

//#define DEBUG

SensirionI2CSgp40 sgp40;
SensirionI2CScd4x scd4x;

// Bluetooth® Low Energy Sensor Service
BLEService sensorService("19B10001-E8F2-537E-4F6C-FFFF768A1214");
// Bluetooth® Low Energy characteristics
BLEFloatCharacteristic temperatureChar("AAAA",BLERead|BLENotify);
BLEFloatCharacteristic humidityChar("BBBB",BLERead|BLENotify);
BLEUnsignedLongCharacteristic co2Char("CCCC",BLERead|BLENotify);
BLEUnsignedLongCharacteristic vocChar("DDDD",BLERead|BLENotify);

long previousMillis = 0;  // last time the battery level was checked, in ms

bool initSensors(void);
bool updateSensors(uint16_t* co2, float* temperature, float* humidity, uint16_t* srawVOC);
void printUint16Hex(uint16_t value);
void printSerialNumber(uint16_t serial0, uint16_t serial1, uint16_t serial2);


void setup() {

    Serial.begin(115200);
    #ifdef DEBUG
    while(!Serial);
    #endif

    if(initSensors()){
        Serial.println("Error while intializing sensors");
    }

    // begin initialization
    if (!BLE.begin()) {
        Serial.println("starting BLE failed!");
        while (1);
    }

    BLE.setLocalName("Hack it back");
    BLE.setAdvertisedService(sensorService); // add the service UUID

    sensorService.addCharacteristic(temperatureChar); // add the battery level characteristic
    sensorService.addCharacteristic(humidityChar); // add the battery level characteristic
    sensorService.addCharacteristic(co2Char); // add the battery level characteristic
    sensorService.addCharacteristic(vocChar); // add the battery level characteristic

    BLE.addService(sensorService); // Add the battery service
    temperatureChar.writeValue(23.7); // set initial value for this characteristic
    humidityChar.writeValue(53); // set initial value for this characteristic
    co2Char.writeValue(3432); // set initial value for this characteristic
    vocChar.writeValue(32023); // set initial value for this characteristic

    Serial.println("Bluetooth® device active, waiting for connections...");

    // start advertising
    BLE.advertise();

}

void loop() {
    uint16_t co2,srawVOC;
    float temperature,humidity;
    // wait for a Bluetooth® Low Energy central
    BLEDevice central = BLE.central();

    // if a central is connected to the peripheral:
    if (central) {
        #ifdef DEBUG
        Serial.print("Connected to central: ");
        Serial.println(central.address());
        digitalWrite(LED_BUILTIN, HIGH);
        #endif
        // check the battery level every 200ms
        // while the central is connected:
        while (central.connected()) {
            long currentMillis = millis();
            // if 5s have passed, check the battery level:
            if (currentMillis - previousMillis >= 5000) {
                previousMillis = currentMillis;
                updateSensors(&co2,&temperature,&humidity,&srawVOC);
            }
        }
        // when the central disconnects, turn off the LED:
        #ifdef DEBUG
        digitalWrite(LED_BUILTIN, LOW);
        Serial.print("Disconnected from central: ");
        Serial.println(central.address());
        #endif
    }
    
}

//Return 1 = error 
//Return 0 = ok 
//Read Sensor each 5s
bool updateSensors(uint16_t* co2, float* temperature, float* humidity, uint16_t* srawVOC){
    uint16_t error;
    char errorMessage[256];
    uint16_t defaultRh = 0x8000;
    uint16_t defaultT = 0x6666;
    uint16_t hum,temp;
    error = sgp40.measureRawSignal(defaultRh, defaultT, *srawVOC);
    if (error) {
        #ifdef DEBUG
        Serial.print("Error trying to execute measureRawSignal(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
        #endif
        return 1;
    } else {
        #ifdef DEBUG
        Serial.print("SRAW_VOC:");
        Serial.println(*srawVOC);
        #endif
    }
    error = scd4x.readMeasurement(*co2, *temperature, *humidity);
    if (error) {
        #ifdef DEBUG
        Serial.print("Error trying to execute readMeasurement(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
        #endif
        return 1;
    } else if (*co2 == 0) {
        #ifdef DEBUG
        Serial.println("Invalid sample detected, skipping.");
        #endif
        return 1;
    } else {
        #ifdef DEBUG
        Serial.print("Co2:");
        Serial.print(*co2);
        Serial.print("\t");
        Serial.print("Temperature:");
        Serial.print(*temperature);
        Serial.print("\t");
        Serial.print("Humidity:");
        Serial.println(*humidity);
        #endif
    }
    //Update service Values
    temperatureChar.writeValue(*temperature);
    humidityChar.writeValue(*humidity);
    co2Char.writeValue(*co2); 
    vocChar.writeValue(*srawVOC);
    return 0;
}

//Return 1 = error 
//Return 0 = ok 
bool initSensors(void){
    
    Wire.begin();

    uint16_t error;
    char errorMessage[256];

    sgp40.begin(Wire);

    uint16_t serialNumber[3];
    uint8_t serialNumberSize = 3;

    error = sgp40.getSerialNumber(serialNumber, serialNumberSize);

    if (error) {
        #ifdef DEBUG
        Serial.print("Error trying to execute getSerialNumber(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
        #endif
        return 1;
    } else {
        #ifdef DEBUG
        Serial.print("SerialNumber:");
        Serial.print("0x");
        for (size_t i = 0; i < serialNumberSize; i++) {
            uint16_t value = serialNumber[i];
            Serial.print(value < 4096 ? "0" : "");
            Serial.print(value < 256 ? "0" : "");
            Serial.print(value < 16 ? "0" : "");
            Serial.print(value, HEX);
        }
        Serial.println();
        #endif
    }

    uint16_t testResult;
    error = sgp40.executeSelfTest(testResult);
    if (error) {
        #ifdef DEBUG
        Serial.print("Error trying to execute executeSelfTest(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
        #endif
        return 1;
    } else if (testResult != 0xD400) {
        #ifdef DEBUG
        Serial.print("executeSelfTest failed with error: ");
        Serial.println(testResult);
        #endif
    }

    scd4x.begin(Wire);

    // stop potentially previously started measurement
    error = scd4x.stopPeriodicMeasurement();
    if (error) {
        #ifdef DEBUG
        Serial.print("Error trying to execute stopPeriodicMeasurement(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
        #endif
        return 1;
    }
    uint16_t serial0;
    uint16_t serial1;
    uint16_t serial2;
    error = scd4x.getSerialNumber(serial0, serial1, serial2);
    if (error) {
        #ifdef DEBUG
        Serial.print("Error trying to execute getSerialNumber(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
        #endif
        return 1;
    } else {
        #ifdef DEBUG
        printSerialNumber(serial0, serial1, serial2);
        #endif
    }

    // Start Measurement
    error = scd4x.startPeriodicMeasurement();
    if (error) {
        #ifdef DEBUG
        Serial.print("Error trying to execute startPeriodicMeasurement(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
        #endif
        return 1;
    }

    Serial.println("Waiting for first measurement... (5 sec)");
    delay(1000);
    return 0;
}

void printUint16Hex(uint16_t value) {
    Serial.print(value < 4096 ? "0" : "");
    Serial.print(value < 256 ? "0" : "");
    Serial.print(value < 16 ? "0" : "");
    Serial.print(value, HEX);
}

void printSerialNumber(uint16_t serial0, uint16_t serial1, uint16_t serial2) {
    Serial.print("Serial: 0x");
    printUint16Hex(serial0);
    printUint16Hex(serial1);
    printUint16Hex(serial2);
    Serial.println();
}
