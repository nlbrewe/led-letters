//LED - Letters
#include <Adafruit_NeoPixel.h>
#include<EEPROM.h>_
#ifdef _AVR_
#include <avr/power.h>
#endif

#define NOTIFY_PIN 4
#define IR_PIN 2      //Sensor Pin 1 wired through a 220 ohm resistor
#define LED_PIN 13     //"Ready to Recieve" flag, not needed but nice

int debug = 0;        //Serial Connection must be started to debug
int start_bit = 2000; //Start bit threshold (Microseconds)
int bin_1 = 1000;     //Binary 1 threshold (Microseconds)
int bin_0 = 400;      //Binary 0 threshold (Microseconds)

volatile int key = -1; // key 137 is "O", default to off

void setup() {
  pinMode(LED_PIN, OUTPUT); //This shows when we're ready to recieve
  pinMode(IR_PIN, INPUT);
  pinMode(NOTIFY_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  Serial.begin(9600);
}

void loop() {
  key = getIRKey();
  if(key == 920){
    digitalWrite(NOTIFY_PIN, HIGH);
    delay(1000);
    digitalWrite(NOTIFY_PIN, LOW);
  }
}


//-------------------------------------------------------------------------------------------------------
int getIRKey() {
  int data[12];
  digitalWrite(LED_PIN, HIGH); //Ok, i'm ready to recieve
  while (pulseIn(IR_PIN, LOW) < 2200) { //Waitfor a start bit
  }
  data[0] = pulseIn(IR_PIN, LOW); //Stat measuring bits, I only want low pulses
  data[1] = pulseIn(IR_PIN, LOW);
  data[2] = pulseIn(IR_PIN, LOW);
  data[3] = pulseIn(IR_PIN, LOW);
  data[4] = pulseIn(IR_PIN, LOW);
  data[5] = pulseIn(IR_PIN, LOW);
  data[6] = pulseIn(IR_PIN, LOW);
  data[7] = pulseIn(IR_PIN, LOW);
  data[8] = pulseIn(IR_PIN, LOW);
  data[9] = pulseIn(IR_PIN, LOW);
  data[10] = pulseIn(IR_PIN, LOW);
  data[11] = pulseIn(IR_PIN, LOW);
  digitalWrite(LED_PIN, LOW);
  if (debug == 1) {
    Serial.println("-------");
  }
  for (int i = 0; i < 11; i++) { //Parse them
    if (debug == 1) {
      Serial.println(data[i]);
    }
    if (data[i] > bin_1) { //is it a 1?
      data[i] = 1;
    } else {
      if (data[i] > bin_0) { //is it a O ?
        data[i] = 0;
      } else {
        data[i] = 2; //  Flag the data as invalid; I don't know what it is!
      }
    }
  }
  for (int i = 0; i < 11; i++) { //Pre—check data for errors
    if (data[i] > 1) {
      Serial.println("getIRKey data[i] > 1.  Returning -1");
      return -1;
    }
  }
  int result = 0;
  int seed = 1;
  for (int i = 0; i < 11; i++) { //Convert bits to integer
    if (data[i] == 1) {
      result += seed;
    }
    seed = seed * 2;
  }
  return result; //Return key number
}


