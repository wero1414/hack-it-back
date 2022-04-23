#define zero 15
#define triac 2


void setup()
{
  pinMode(triac, OUTPUT);
  pinMode(zero, INPUT);
  attachInterrupt(zero, angle, RISING);
}

void loop(){}

void angle(){ //8.33mS 
  digitalWrite(triac, LOW);
  delayMicroseconds(4000);
  digitalWrite(triac, HIGH);
}
