#include <ArduinoBLE.h>

#define zeroCross 15
#define triacOutput 2
#define DEBUG

long previousMillis,delayTime = 0;  // last time the battery level was checked, in ms

union floatToInt
{
    int32_t intMember;
    float floatMember; /* Float must be 32 bits IEEE 754 for this to work */
};

float temperatureGlobal,humidityGlobal;
uint16_t co2Global,vocGlobal;

void updateDelayTime();
void explorerPeripheral(BLEDevice);
void exploreService(BLEService);
void exploreCharacteristic(BLECharacteristic);
void zeroCrossISR();


void setup() {
  #ifdef DEBUG
  Serial.begin(9600);
  while (!Serial);
  #endif
  
  pinMode(triacOutput, OUTPUT);
  pinMode(zeroCross, INPUT);
  attachInterrupt(zeroCross, zeroCrossISR, RISING);
  
  // begin initialization
  if (!BLE.begin()) {
    Serial.println("starting Bluetooth® Low Energy module failed!");
    while (1);
  }

  Serial.println("Bluetooth® Low Energy Central - Peripheral Explorer");

  // start scanning for peripherals
  BLE.scan();
}

void loop() {
  // check if a peripheral has been discovered
  BLEDevice peripheral = BLE.available();

  if (peripheral) {
    // see if peripheral is a Hack it back node
    if (peripheral.localName() == "Hack it back") {
      // stop scanning
      BLE.stopScan();
      #ifdef DEBUG
      // discovered a peripheral, print out address, local name, and advertised service
      Serial.print("Found ");
      Serial.print(peripheral.address());
      Serial.print(" '");
      Serial.print(peripheral.localName());
      Serial.print("' ");
      Serial.print(peripheral.advertisedServiceUuid());
      Serial.println();
      #endif 
      //uC get cycled once connected to one node here
      explorerPeripheral(peripheral); 
      BLE.scan();
    }
  }
}

void explorerPeripheral(BLEDevice peripheral) {
  if (peripheral.connect()) {
    Serial.println("Connected");
  } else {
    Serial.println("Failed to connect!");
    return;
  }
  while (peripheral.connected()) {
    long currentMillis = millis();
    if (currentMillis - previousMillis >= 5000) {
      previousMillis =millis();
      if (peripheral.discoverAttributes()) {
        Serial.println("Attributes discovered");
      } else {
        Serial.println("Attribute discovery failed!");
        return;
      }
      BLEService service = peripheral.service(2);
      exploreService(service);
      updateDelayTime();
      }
  }
  // we are done exploring, disconnect
  #ifdef DEBUG
  Serial.println("Disconnecting ...");
  #endif
  peripheral.disconnect();
  #ifdef DEBUG
  Serial.println("Disconnected");
  #endif
}

void exploreService(BLEService service) {
  // loop the characteristics of the service and explore each
  for (int i = 0; i < service.characteristicCount(); i++) {
    BLECharacteristic characteristic = service.characteristic(i);
    exploreCharacteristic(characteristic);
  }
}

void exploreCharacteristic(BLECharacteristic characteristic) {
   const char* uuidChar = characteristic.uuid();
   String uuidStr="";
   for(int i=0;i<4;i++){
     uuidStr += String(uuidChar[i]);
   }
  
   if(uuidStr=="aaaa"){
      union floatToInt temperature;
      if (characteristic.canRead()) {
        uint32_t value = 0;
        // read the characteristic value
        characteristic.read();
        characteristic.readValue(value);
        temperature.intMember=value;
        temperatureGlobal=temperature.floatMember;        
        #ifdef DEBUG
        Serial.println("UUID de temperatura");
        Serial.print("value es: ");
        Serial.println(temperature.floatMember);
        #endif
      }
   }
   else if(uuidStr=="bbbb"){
      union floatToInt humidity;
      if (characteristic.canRead()) {
        uint32_t value = 0;
        // read the characteristic value
        characteristic.read();
        characteristic.readValue(value);
        humidity.intMember=value;
        humidityGlobal=humidity.floatMember;
        #ifdef DEBUG
        Serial.println("UUID de humedad");
        Serial.print("value es: ");
        Serial.println(humidity.floatMember);
        #endif
      }
   }
   else if(uuidStr=="cccc"){
      if (characteristic.canRead()) {
        uint32_t value = 0;
        characteristic.read();
        characteristic.readValue(value);
        co2Global=value;
        #ifdef DEBUG
        Serial.println("UUID de co2");
        Serial.print("value es: ");
        Serial.println((uint16_t)value);
        #endif
      }
   }
   else if(uuidStr=="dddd"){
      if (characteristic.canRead()) {
        uint32_t value = 0;
        // read the characteristic value
        characteristic.read();
        characteristic.readValue(value);
        vocGlobal=value;
        #ifdef DEBUG
        Serial.println("UUID de voc");
        Serial.print("value es: ");
        Serial.println((uint16_t)value);
        #endif
      }
   }
}

void updateDelayTime(){
  if(temperatureGlobal>34.0 | co2Global>800 | vocGlobal > 60000)
  delayTime=2083*0; //100% power
  else if(temperatureGlobal>30.0 | co2Global>600 | vocGlobal > 50000)
  delayTime=2083;   //75% power
  else if(temperatureGlobal>25.0 | co2Global>500 | vocGlobal > 30000)
  delayTime=4166;   //50% power
  else if(temperatureGlobal<25.0 | co2Global<500 | vocGlobal < 30000)
  delayTime=6249;   //25% power
  #ifdef DEBUG
  Serial.print("delay time");
  Serial.println(delayTime);
  #endif
  }

void zeroCrossISR() { //8.33mS
  digitalWrite(triacOutput, LOW);
  delayMicroseconds(delayTime); //Check Timeout BLE
  digitalWrite(triacOutput, HIGH);
}
