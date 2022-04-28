#include <ArduinoBLE.h>

#define zeroCross 15
#define triacOutput 2

#define nodesToRead 5
#define DEBUG

String addressRead[nodesToRead];
int i=0;
bool scanFlag=0;

long previousMillis = 0;  // last time the battery level was checked, in ms
int peripheralCounter=0;

void zeroCrossISR();

void setup() {
  pinMode(triacOutput, OUTPUT);
  pinMode(zeroCross, INPUT);
  attachInterrupt(zeroCross, zeroCrossISR, RISING);
  
  Serial.begin(115200);
  while (!Serial);

  // begin initialization
  if (!BLE.begin()) {
    Serial.println("starting Bluetooth® Low Energy module failed!");
    while (1);
  }

  Serial.println("Bluetooth® Low Energy Central - Peripheral Explorer");
  BLE.scan();
}

void loop() {
  //Check every 5 min all the nodes
  long currentMillis = millis();
  if (currentMillis - previousMillis >= 5000) {
    Serial.println("5 seconds");
    peripheralCounter=0;
    previousMillis = currentMillis;
    for(int i=0;i<nodesToRead;i++){
        BLE.stopScan();
        addressRead[i]="";
        BLE.scan();
        scanFlag=1;
    }
  }

  if(scanFlag){
    // check if a peripheral has been discovered
    BLEDevice peripheral = BLE.available();
    
    if (peripheral) {
      // see if peripheral is a LED
      if (peripheral.localName() == "Hack it back") {
        #ifdef DEBUG
        Serial.print("Found ");
        Serial.print(peripheral.address());
        Serial.print(" '");
        Serial.print(peripheral.localName());
        Serial.print("' ");
        Serial.print(peripheral.advertisedServiceUuid());
        Serial.println();
        #endif
        
        String actualAddress = peripheral.address();
        bool peripheralRepeted=0;
        for(int a=0;a<nodesToRead;a++){
           if(actualAddress.equals(addressRead[a])){
            peripheralRepeted=1;
            peripheralCounter++;
            if(peripheralCounter==nodesToRead)scanFlag=0;
           }
          }
        // stop scanning
        if(!peripheralRepeted){
          BLE.stopScan();
          addressRead[i]=peripheral.address();
          explorerPeripheral(peripheral);
          i++;
          BLE.scan();
        }
      }
    }
  }
}

void zeroCrossISR(){ //8.33mS 
  digitalWrite(triacOutput, LOW);
  //delayMicroseconds(4000); //Check Timeout BLE
  digitalWrite(triacOutput, HIGH);
}

void explorerPeripheral(BLEDevice peripheral) {
  // connect to the peripheral
  Serial.println("Connecting ...");

  if (peripheral.connect()) {
    Serial.println("Connected");
  } else {
    Serial.println("Failed to connect!");
    return;
  }

  // discover peripheral attributes
  Serial.println("Discovering attributes ...");
  if (peripheral.discoverAttributes()) {
    Serial.println("Attributes discovered");
  } else {
    Serial.println("Attribute discovery failed!");
    peripheral.disconnect();
    return;
  }

  // read and print device name of peripheral
  Serial.println();
  Serial.print("Device name: ");
  Serial.println(peripheral.deviceName());
  Serial.print("Appearance: 0x");
  Serial.println(peripheral.appearance(), HEX);
  Serial.println();

  // loop the services of the peripheral and explore each
  for (int i = 0; i < peripheral.serviceCount(); i++) {
    BLEService service = peripheral.service(i);

    exploreService(service);
  }

  Serial.println();

  // we are done exploring, disconnect
  Serial.println("Disconnecting ...");
  peripheral.disconnect();
  Serial.println("Disconnected");
}

void exploreService(BLEService service) {
  // print the UUID of the service
  Serial.print("Service ");
  Serial.println(service.uuid());

  // loop the characteristics of the service and explore each
  for (int i = 0; i < service.characteristicCount(); i++) {
    BLECharacteristic characteristic = service.characteristic(i);

    exploreCharacteristic(characteristic);
  }
}

void exploreCharacteristic(BLECharacteristic characteristic) {
  // print the UUID and properties of the characteristic
  Serial.print("\tCharacteristic ");
  Serial.print(characteristic.uuid());
  Serial.print(", properties 0x");
  Serial.print(characteristic.properties(), HEX);

  // check if the characteristic is readable
  if (characteristic.canRead()) {
    // read the characteristic value
    characteristic.read();

    if (characteristic.valueLength() > 0) {
      // print out the value of the characteristic
      Serial.print(", value 0x");
      printData(characteristic.value(), characteristic.valueLength());
    }
  }
  Serial.println();

  // loop the descriptors of the characteristic and explore each
  for (int i = 0; i < characteristic.descriptorCount(); i++) {
    BLEDescriptor descriptor = characteristic.descriptor(i);

    exploreDescriptor(descriptor);
  }
}

void exploreDescriptor(BLEDescriptor descriptor) {
  // print the UUID of the descriptor
  Serial.print("\t\tDescriptor ");
  Serial.print(descriptor.uuid());

  // read the descriptor value
  descriptor.read();

  // print out the value of the descriptor
  Serial.print(", value 0x");
  printData(descriptor.value(), descriptor.valueLength());

  Serial.println();
}

void printData(const unsigned char data[], int length) {
  for (int i = 0; i < length; i++) {
    unsigned char b = data[i];

    if (b < 16) {
      Serial.print("0");
    }

    Serial.print(b, HEX);
  }
}
