//LED - Letters
#include <Adafruit_NeoPixel.h>
#include<EEPROM.h>_
#ifdef _AVR_
#include <avr/power.h>
#endif
#include "EEPROM_Writeanything.h"
#define NEO_PIN 6 //pin driving neo_pixe1 string
#define NOTIFY_PIN 4
#define IR_PIN 2      //Sensor Pin 1 wired through a 220 ohm resistor
#define LED_PIN 13     //"Ready to Recieve" flag, not needed but nice
#define NUMPIXELS 150

int debug = 0;        //Serial Connection must be started to debug
int start_bit = 2000; //Start bit threshold (Microseconds)
int bin_1 = 1000;     //Binary 1 threshold (Microseconds)
int bin_0 = 400;      //Binary 0 threshold (Microseconds)

volatile int key = -1; // key 137 is "O", default to off
float Hue;
float Saturation;
float Intensity;
uint8_t Pattern = 0;
#define PATTERN_None 0  //all LEDs off
#define PATTERN_Solid 1 //solid LEDs
#define PATTERN_Rainbow 2  //rainbow LEDs
#define PATTERN_Random 3  //random LEDs

uint8_t Rotation = 0;
#define ROTATION_None 0  //no movement
#define ROTATION_Right 1  //rotate right
#define ROTATION_Left 2  //rotate left
#define ROTATION_Random 3  //make colors cycle randomly
#define ROTATION_Pack 4  //
uint8_t OpMode = 0; //0=stop, 1=play
#define OpMode_Stop 0
#define OpMode_Play 1
#define OpMode_Interrupted 2
bool blnHasChanges = false;
unsigned int baseAddr = 0;

//map for eeprom data
byte adrMode = baseAddr;
byte adrRotation = adrMode + sizeof(Pattern);
byte adrHue = adrRotation + sizeof(Rotation);
byte adrIntensity = adrHue + sizeof(Hue);
byte adrSaturation = adrIntensity + sizeof(Intensity);
bool initPass = true;
bool blnInterrupted = false;

// Parameter 1 = number of pixels/in Strip
// Parameter 2 = Arduino pin numhbr (most are valid)
// Parameter 3 = pixel type flag, add tcgether as needed:
// NEO_KHZ800 800 KHZ bitstream (most NeoPixel products w/WS28l2 LEDs)
// NEO_KHZ4OO 400 KHZ (Classic 'vl' (not v2) FLORA pixels, WS28l1 drivers)
// NEO_GRB Pixels are wired for GRB bitstream (most NeoPixel products)
// NEO_RGB Pixels are wired for RGB bitstream (vl FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMPIXELS, NEO_PIN, NEO_GRB + NEO_KHZ800);
uint16_t HueArray[NUMPIXELS];  //Hue storage
uint16_t HueArray2[NUMPIXELS];  //Hue storage

void setup() {
  pinMode(NEO_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT); //This shows when we're ready to recieve
  pinMode(IR_PIN, INPUT);
  pinMode(NOTIFY_PIN, INPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.begin(9600);
  int n = 0;
  //  n += EEPROM_writeAnything(adrMode, 0);
  //  n += EEPROM_writeAnything(adrRotation, 0);
  //  n += EEPROM_writeAnything(adrHue, 120.0);
  //  n += EEPROM_writeAnything(adrIntensity, 0.99);
  //  n += EEPROM_writeAnything(adrSaturation, 0.99);
  n += EEPROM_readAnything(baseAddr, Pattern);
  n += EEPROM_readAnything(adrRotation, Rotation);
  n += EEPROM_readAnything(adrHue, Hue);
  n += EEPROM_readAnything(adrIntensity, Intensity);
  n += EEPROM_readAnything(adrSaturation, Saturation);
  Serial.print("EEPROM read, bytes: ");
  Serial.println(n);
  Serial.print(" Pattern: ");
  Serial.print(Pattern);
  Serial.print(" Rotation: ");
  Serial.print(Rotation);
  Serial.print(" Hue: ");
  Serial.print(Hue);
  Serial.print(" Saturation: ");
  Serial.print(Saturation);
  Serial.print(" Intensity: ");
  Serial.println(Intensity);
  strip.begin();
  strip.show();
  initPass = true;
}

void loop() {
  int n = 0;
  if (digitalRead(NOTIFY_PIN)) {  //LED pin went low and caused interrupt which set NOTIFY_PIN
    Serial.println("Notify pin set ");
    key = -1;
    OpMode = OpMode_Interrupted;  //flag status as interrupted
    delay(1010);
  } else {  //wait for IR input
    OpMode = OpMode_Stop;
    Serial.println("Notify pin not set ");
    if ( ! initPass) {
      if (OpMode == OpMode_Interrupted) {
        OpMode = OpMode_Stop;
      }
      key = getIRKey();  //get key
      Serial.print("Key pressed: ");
      Serial.println(key);
    } else {
      OpMode = OpMode_Play;
      initPass = false;
      Serial.println("Initial Pass - Starting PLAY mode");
    }
    switch (key) {
      case 128: //l key no lights on
        Pattern = PATTERN_None;
        for (uint16_t i = 0; i < strip.numPixels(); i++) {
          strip.setPixelColor(i , 0); //turn off all pixels
        }
        strip.show();
        blnHasChanges = true;
        break;
      case 129: //2 key solid color. HSI select color
        Pattern = PATTERN_Solid;
        blnHasChanges = true;
        colorWipeHSI(Hue, Saturation, Intensity, 5);
        break;
      case 130: //3 key rainbow
        Pattern = PATTERN_Rainbow;
        blnHasChanges = true;
        rainbowHSI(Hue, Saturation, Intensity, 5);
        break;
      case 131: //4 random colors
        blnHasChanges = true;
        Rotation = ROTATION_Random;
        Random(Hue, Saturation, Intensity, 5);
        break;
      case 132: //5 rotate right (ccw)
        blnHasChanges = true;
        Rotation = ROTATION_Right;
        break;
      case 133: //6 rotate left (cw)
        blnHasChanges = true;
        Rotation = ROTATION_Left;
        break;
      case 134: //7 random
        blnHasChanges = true;
        Rotation = ROTATION_Random;
        break;
      case 135: //8 random
        blnHasChanges = true;
        Rotation = ROTATION_Pack;
        break;
      case 144: //ch + change color
        blnHasChanges = true;
        Hue = Hue + 1;
        if (Hue > 360)Hue = 0.0;
        Serial.print("co1or: ");
        Serial.println(Hue);
        colorWipeHSI(Hue, 0.99, 0.99, 5);
        strip.show();
        break;
      case 145: //ch - change color
        blnHasChanges = true;
        Hue = Hue - 1;
        if (Hue < 0.0)Hue = 360;
        Serial.print("co1or: ");
        Serial.println(Hue);
        colorWipeHSI(Hue, 0.99, 0.99, 5);
        strip.show();
        break;
      case 147: //vo1-  decrease intensity
        blnHasChanges = true;
        Intensity = max(Intensity - 0.10, .001);
        Serial.print("Intensity: ");
        Serial.println(Intensity);
        colorWipeHSI(Hue, Saturation, Intensity, 5);
        strip.show();
        break;
      case 146: //vo1+  increase intensity
        blnHasChanges = true;
        Intensity = min( Intensity + 0.05, 0.999 );
        Serial.print("Intensity: ");
        Serial.println(Intensity);
        colorWipeHSI(Hue, Saturation, Intensity, 5);
        strip.show();
        break;
      case 923: //<<  decrease saturation
        blnHasChanges = true;
        Saturation = max(Saturation - 0.05, .001);
        Serial.print("Saturation: ");
        Serial.println(Saturation);
        colorWipeHSI(Hue, Saturation, Intensity, 5);
        break;
      case 924:  //>>  increase saturation
        blnHasChanges = true;
        Saturation = min( Saturation + 0.10, .999);
        Serial.print("Saturation: ");
        Serial.println(Saturation);
        colorWipeHSI(Hue, Saturation, Intensity, 5);
        break;
      case 148: //OK button — write out values to eeprom
        if (blnHasChanges) {
          blnHasChanges = false;
          n = 0;
          n += EEPROM_writeAnything(adrMode, Pattern);
          n += EEPROM_writeAnything(adrRotation, Rotation);
          n += EEPROM_writeAnything(adrHue, Hue);
          n += EEPROM_writeAnything(adrIntensity, Intensity);
          n += EEPROM_writeAnything(adrSaturation, Saturation);
          Serial.print("EEPOM write, bytes: ");
          Serial.println(n);
        }
        break;
      case 920: //stop
        Serial.print("STOP pressed: ");
        OpMode = OpMode_Stop;
        break;
      case -1:
      case -2:
        Serial.print("No  valid key pressed after interrupt, resuming PLAY");
        OpMode = OpMode_Play;
        break;
      case 922: //play
        Serial.print("PLAY pressed: ");
        OpMode = OpMode_Play;
        //delay(1000);
        break;
    }
    Serial.print(" Pattern: ");
    Serial.print(Pattern);
    Serial.print(" Rotation: ");
    Serial.print(Rotation);
    Serial.print(" Hue: ");
    Serial.print(Hue);
    Serial.print(" Saturation: ");
    Serial.print(Saturation);
    Serial.print(" Intensity: ");
    Serial.println(Intensity);
    Serial.print(" OpMod: ");
    Serial.println(OpMode);
    if (OpMode == OpMode_Play) {
      switch (Pattern) {
        case PATTERN_None:
          break;
        case PATTERN_Solid:
          switch (Rotation) {
            case ROTATION_None:
              Serial.println("colorWipeHSI");
              colorWipeHSI(Hue, Saturation, Intensity, 5);
              break;
            case ROTATION_Right:
              ColorFlow(Hue, Saturation , Intensity, 125);
              break;
              Serial.println("theaterChase");
              theaterChase(Hue, Saturation, Intensity, 50);
              break;
            case ROTATION_Left:
              Serial.println("theaterChaseRev");
              theaterChaseRev(Hue, Saturation , Intensity, 50);
              break;
            case ROTATION_Random:
              colorWipeHSIRandom(Hue, Saturation, Intensity, 5);
              break;
            case ROTATION_Pack:
              Pack(Hue, Saturation, Intensity, 5);
              break;
          }
          break;
        case PATTERN_Rainbow:
          switch (Rotation) {
            case ROTATION_None:
              Serial.println("rainbowHSI");
              rainbowHSI(Hue, Saturation, Intensity, 5);
              break;
            case ROTATION_Right:
              ColorFlow(Hue, Saturation , Intensity, 125);
              break;
              Serial.println("rainbowCycleHSI");
              rainbowCycleHSI(Hue, Saturation, Intensity, 30);
              break;
            case ROTATION_Left:
              Serial.println("rainbowCycleHSIRev");
              rainbowCycleHSIRev(Hue, Saturation, Intensity, 30);
              break;
            case ROTATION_Random:
              RainbowRandom(Hue, Saturation, Intensity, 5);
              break;
            case ROTATION_Pack:
              RainbowPack(Hue, Saturation, Intensity, 5);
              break;
          }  //end rainbow switch on rotation
          break;
        case PATTERN_Random:
          switch (Rotation) {
            case ROTATION_None:
              Serial.println("rainbowHSI");
              rainbowHSI(Hue, Saturation, Intensity, 5);
              break;
            case ROTATION_Right:
              ColorFlow(Hue, Saturation , Intensity, 125);
              break;
              Serial.println("rainbowCycleHSI");
              rainbowCycleHSI(Hue, Saturation, Intensity, 30);
              break;
            case ROTATION_Left:
              Serial.println("rainbowCycleHSIRev");
              rainbowCycleHSIRev(Hue, Saturation, Intensity, 30);
              break;
            case ROTATION_Random:
              RainbowRandom(Hue, Saturation, Intensity, 5);
              break;
            case ROTATION_Pack:
              RainbowPack(Hue, Saturation, Intensity, 5);
              break;
          }  //end switch on Pattern
          break;
      }
    }
  }
}
//-------------------------------------------------------------------------------------------------------
// Fill individual pixels with current HSI.  Nonblocking
void colorWipeHSI( float H, float S, float I, uint8_t wait) {
  int rgb[3];
  uint32_t c;
  hsi2rgb( H, S, I, rgb);
  c = strip.Color(rgb[0], rgb[1], rgb[2]); //convert rgb value to 4 byte color value
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    HueArray[i] = H;
    if (digitalRead(NOTIFY_PIN))return;
    strip.setPixelColor(i, c);
    //strip.show();
    //delay(wait);
  }
  strip.setBrightness(255);
  strip.show();
}
//-------------------------------------------------------------------------------------------------------
// Fill individual pixels with random Hue, current S,I.  Nonblocking
void Random( float H, float S, float I, uint8_t wait) {
  int rgb[3];
  uint32_t c;
  randomSeed(analogRead(0));
  for (uint16_t i = 0; i < strip.numPixels() ; i++) {  //pick different Hue for each pixel
    HueArray[i] = random(0, 359);
    hsi2rgb( HueArray[i], S, I, rgb);
    c = strip.Color(rgb[0], rgb[1], rgb[2]); //convert rgb value to 4 byte color value
    strip.setPixelColor(i, c);
  }
  strip.show();
}
//-------------------------------------------------------------------------------------------------------
//Fill individual pixels with all possible colors starting with current H,S,I values.  nonblocking
void rainbowHSI(float H, float S, float I, uint8_t wait) {
  int rgb[3];
  float H0 = H;
  uint32_t c;
  uint16_t i, j;
  for (i = 0; i < strip.numPixels(); i++) {
    if (digitalRead(NOTIFY_PIN))return;
    H = H0 + float(i) * (360.0 / float(strip.numPixels()));
    HueArray[i] = H;
    hsi2rgb( H, S, I, rgb); //fill in rgb vector
    c = strip.Color(rgb[0], rgb[1], rgb[2]); //convert rgb value to 4 byte color value
    strip.setPixelColor(i, c);
  }
  strip.show();
}


//-------------------------------------------------------------------------------------------------------
// make intensity of LED string pulsate from 0-max-0.
void Pulsate( float H, float S, float I, uint8_t wait) {
  int rgb[3];
  uint32_t c;
  for (uint16_t intensity = 0; intensity < 100 ; intensity += 2) {  //cycle entire string from 0 to max intensity
    if (digitalRead(NOTIFY_PIN))return;
    I = float(intensity) / 100;
    for (uint16_t pixel = 0; pixel < strip.numPixels(); pixel++) {
      if (digitalRead(NOTIFY_PIN))return;
      hsi2rgb( HueArray[pixel], S, I, rgb);
      c = strip.Color(rgb[0], rgb[1], rgb[2]);
      strip.setPixelColor(pixel, c);
    }
    strip.show();
    delay(10);
  }
  for (uint16_t intensity = 100; intensity > 0 ; intensity -= 2) {  //cycle entire string max intensity to 0
    if (digitalRead(NOTIFY_PIN))return;
    I = float(intensity) / 100;
    for (uint16_t pixel = 0; pixel < strip.numPixels(); pixel++) {
      if (digitalRead(NOTIFY_PIN))return;
      hsi2rgb( HueArray[pixel], S, I, rgb);
      c = strip.Color(rgb[0], rgb[1], rgb[2]);
      strip.setPixelColor(pixel, c);
    }
    strip.show();
    delay(10);
  }
}

//-------------------------------------------------------------------------------------------------------

//cycle moving colors to new locations
void ColorFlow(float H, float S, float I, uint8_t wait) {
#define MOVEDIST 1
  int rgb[3];
  uint32_t c;
  uint16_t i, j;
  Serial.println("ColorFlow");
  while (1 == 1) {
    if (digitalRead(NOTIFY_PIN))return;
    for (i = 0; i < strip.numPixels() - MOVEDIST; i++) {  //copy old Hues to temp
      HueArray2[i + MOVEDIST] = HueArray[i];
    }
    for (i = 0; i < MOVEDIST; i++) {
      HueArray2[i] = HueArray[strip.numPixels() - MOVEDIST + i];
    }
    for (i = 0; i < strip.numPixels(); i++) {  //save shuffeled hues back
      HueArray[i] = HueArray2[i];
    }
    for (i = 0; i < strip.numPixels(); i++) {
      if (digitalRead(NOTIFY_PIN))return;
      H = float(HueArray[i]);
      hsi2rgb( H, S, I, rgb); //fill in rgb vector
      c = strip.Color(rgb[0], rgb[1], rgb[2]); //convert rgb value to 4 byte color value
      strip.setPixelColor(i, c);
    }
    strip.show();
    delay(wait);
  }
}

//-------------------------------------------------------------------------------------------------------
// Fill all dots with same random Hue,make intensity pulse
void colorWipeHSIRandom( float H, float S, float I, uint8_t wait) {
  int rgb[3];
  uint32_t c;
  randomSeed(analogRead(0));
  while (1 == 1) {
    H = random(0, 359);  //pick a random color for entire string

    for (uint16_t i = 0; i < 100 ; i += 2) {  //cycle entire string from 0 to max intensity
      if (digitalRead(NOTIFY_PIN))return;
      I = float(i) / 100;
      hsi2rgb( H, S, I, rgb);
      c = strip.Color(rgb[0], rgb[1], rgb[2]);
      for (uint16_t i = 0; i < strip.numPixels(); i++) {
        if (digitalRead(NOTIFY_PIN))return;
        strip.setPixelColor(i, c);
      }
      strip.show();
      delay(10);
    }
    for (int i = 100; i > 0 ; i -= 2) {  //now cycle from max to min intensity
      if (digitalRead(NOTIFY_PIN))return;
      I = float(i) / 100;
      hsi2rgb( H, S, I, rgb);
      c = strip.Color(rgb[0], rgb[1], rgb[2]); //convert rgb value to 4 byte color value
      for (uint16_t i = 0; i < strip.numPixels(); i++) {
        if (digitalRead(NOTIFY_PIN))return;
        strip.setPixelColor(i, c);
      }
      strip.show();
      delay(10);
    }
  }
}

//-------------------------------------------------------------------------------------------------------
// Fill individual pixels with random Hue, vary intensity
void RainbowRandom( float H, float S, float I, uint8_t wait) {
  int rgb[3];
  uint32_t c;
  randomSeed(analogRead(0));
  for (int j = 0; j < NUMPIXELS ; j++) {  //pick different Hue for each pixel
    HueArray[j] = random(0, 359);
  }
  while (1 == 1) {
    if (digitalRead(NOTIFY_PIN))return;
    S = .999;
    for (int j = 0; j < 100 ; j += 2) {
      for (uint16_t i = 0; i < strip.numPixels(); i++) {
        H = HueArray[i];
        I = float(j) / 100;
        hsi2rgb( H, S, I, rgb);
        c = strip.Color(rgb[0], rgb[1], rgb[2]); //convert rgb value to 4 byte color value
        if (digitalRead(NOTIFY_PIN))return;
        strip.setPixelColor(i, c);
      }
      strip.show();
    }

    for (int j = 100; j > 0 ; j -= 2) {
      for (uint16_t i = 0; i < strip.numPixels(); i++) {
        H = HueArray[i];
        I = float(j) / 100;
        hsi2rgb( H, S, I, rgb);
        c = strip.Color(rgb[0], rgb[1], rgb[2]); //convert rgb value to 4 byte color value
        if (digitalRead(NOTIFY_PIN))return;
        strip.setPixelColor(i, c);
      }
      strip.show();
    }
  }
}


//-------------------------------------------------------------------------------------------------------
//evenly distribute colors over string, and cycle moving colors to new locations
void rainbowCycleHSI(float H, float S, float I, uint8_t wait) {
  int rgb[3];
  uint32_t c;
  uint16_t i, j;
  while (1 == 1) {
    for (j = 0; j < 360 * 5; j = j + 5) { //rotate colors around string
      for (i = 0; i < strip.numPixels(); i++) {
        if (digitalRead(NOTIFY_PIN))return;
        H = (float(i) * 360.0 / float(strip.numPixels())) + float(j);
        hsi2rgb( H, S, I, rgb); //fill in rgb vector
        c = strip.Color(rgb[0], rgb[1], rgb[2]); //convert rgb value to 4 byte color value
        strip.setPixelColor(i, c);
      }
      strip.show();
      delay(wait);
      if (digitalRead(NOTIFY_PIN))return;
    }
  }
}
//-------------------------------------------------------------------------------------------------------
void rainbowCycleHSIRev(float H, float S, float I, uint8_t wait) {
  //evenly distribute colors over string, and cycle moving colors to new locations reverse direction
  int rgb[3];
  uint32_t c;
  uint16_t i, j ;
  while ( 1 == 1) {
    for (j = 0; j < 360 * 5; j = j + 5) {// 5 cycles of all colors on wheel
      for (i = strip.numPixels(); i > 0; i--) {
        if (digitalRead(NOTIFY_PIN))return;
        H = (float(i) * 360.0 / float(strip.numPixels())) + float(j);
        hsi2rgb( H, S, I, rgb); //fill in rgb vector
        c = strip.Color(rgb[0], rgb[1], rgb[2]); //convert rgb value to 4 byte color value
        strip.setPixelColor(i, c);
      }
      strip.show();
      delay(wait);
      if (digitalRead(NOTIFY_PIN))return;
    }
  }
}
//-------------------------------------------------------------------------------------------------------
//Theatre—style reverse crawling lights
void theaterChaseRev(float H, float S, float I, uint8_t wait) {
  int rgb[3];
  uint32_t c;
  hsi2rgb( H, S, I, rgb);
  c = strip.Color(rgb[0], rgb[1], rgb[2]); //convert rgb value to 4 byte color value
  int q;
  int i;
  while (!digitalRead(NOTIFY_PIN)) { //do this forever
    if (digitalRead(NOTIFY_PIN)) {
      Serial.println("theaterChaseRev returning due to NOTIFY_PIN");
      return;
    }
    for ( q = 0; q < 3; q++) {
      for (i = 0; i < strip.numPixels(); i = i + 3) {
        strip.setPixelColor(i + q, c); //turn every third pixel on
      }
      strip.show();
      delay(wait);
      for (int i = 0; i < strip.numPixels(); i = i + 3) {
        strip.setPixelColor(i + q, 0); //turn every third pixel off
        if (digitalRead(NOTIFY_PIN)) {
          Serial.println("theaterChaseRev returning due to NOTIFY_PIN");
          return;
        }
      }
    }
  }
}
//-------------------------------------------------------------------------------------------------------
//Theatre—style crawling lights.
void theaterChase(float H, float S, float I, uint8_t wait) {
  int rgb[3];
  uint32_t c;
  hsi2rgb( H, S, I, rgb);
  c = strip.Color(rgb[0], rgb[1], rgb[2]); //convert rgb value to 4 byte color value
  int q;
  int i;
  while (! digitalRead(NOTIFY_PIN)) { //do this forever
    if (digitalRead(NOTIFY_PIN)) {
      Serial.println("theaterChase returning due to NOTIFY_PIN");
      return;
    }
    for ( q = 3; q > 0; q--) {
      for (i = 0; i < strip.numPixels(); i = i + 3) {
        if (digitalRead(NOTIFY_PIN)) {
          Serial.println("theaterChase returning due to NOTIFY_PIN");
          return;
        }
        strip.setPixelColor(i + q, c); //turn every third pixel on
      }
      strip.show();
      delay(wait);
      for (int i = 0; i < strip.numPixels(); i = i + 3) {
        if (digitalRead(NOTIFY_PIN)) {
          Serial.println("theaterChase returning due to NOTIFY_PIN");
          return;
        }
        strip.setPixelColor(i + q, 0); //turn every third pixel off
      }
    }
  }
}

//-------------------------------------------------------------------------------------------------------
//Pack of chasing pixels
void Pack(float H, float S, float I, uint8_t wait) {
  int rgb[3];
  uint32_t c;
  uint16_t j;
  uint16_t i;
  hsi2rgb( H, S, I, rgb);
  Serial.println("Pack");
  c = strip.Color(rgb[0], rgb[1], rgb[2]); //convert rgb value to 4 byte color value
  Serial.println(c);
  for ( i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, 0); //turn remaining pixels
    if (digitalRead(NOTIFY_PIN)) {
      Serial.println("Pack() returning due to NOTIFY_PIN");
      return;
    }
  }
  strip.show();
  while (!digitalRead(NOTIFY_PIN)) { //do this forever
    if (digitalRead(NOTIFY_PIN)) {
      Serial.println("Pack() returning due to NOTIFY_PIN");
      return;
    }
    for ( j = 0; j < strip.numPixels() - 4; j++) {

      for ( i = 0; i < j; i++) {
        strip.setPixelColor(i, 0); //turn off leading pixels
        if (digitalRead(NOTIFY_PIN)) {
          Serial.println("Pack() returning due to NOTIFY_PIN");
          return;
        }
      }

      for (i = j; i < j + 4; i++) {
        if (digitalRead(NOTIFY_PIN)) {
          Serial.println("Pack() returning due to NOTIFY_PIN");
          return;
        }
        strip.setPixelColor(i, c);  //turn on pack of pixels
      }

      for ( i = j + 4; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, 0); //turn remaining pixels
        if (digitalRead(NOTIFY_PIN)) {
          Serial.println("Pack() returning due to NOTIFY_PIN");
          return;
        }
      }
      strip.show();
      delay(20);
    } //end j loop

    for ( j = strip.numPixels(); j > 4; j--) {

      for ( i = 0; i < j; i++) {
        strip.setPixelColor(i, 0); //turn off leading pixels
        if (digitalRead(NOTIFY_PIN)) {
          Serial.println("Pack() returning due to NOTIFY_PIN");
          return;
        }
      }

      for (i = j - 4; i < j; i++) {
        if (digitalRead(NOTIFY_PIN)) {
          Serial.println("Pack() returning due to NOTIFY_PIN");
          return;
        }
        strip.setPixelColor(i, c);  //turn on pack of pixels
      }

      for ( i = j; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, 0); //turn remaining pixels
        if (digitalRead(NOTIFY_PIN)) {
          Serial.println("Pack() returning due to NOTIFY_PIN");
          return;
        }
      }
      strip.show();
      delay(20);
    } //end j loop
  }
}

//-------------------------------------------------------------------------------------------------------
//Pack of chasing pixels in all colors
void RainbowPack(float H, float S, float I, uint8_t wait) {
  int rgb[3];
  uint32_t c;
  uint16_t j;
  uint16_t i;

  randomSeed(analogRead(0));
  for ( j = 0; j < NUMPIXELS ; j++) {  //pick different Hue for each pixel
    HueArray[j] = random(0, 359);
    if (digitalRead(NOTIFY_PIN)) {
      Serial.println("Pack() returning due to NOTIFY_PIN");
      return;
    }
  }

  for ( i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, 0); //turn remaining pixels
    if (digitalRead(NOTIFY_PIN)) {
      Serial.println("Pack() returning due to NOTIFY_PIN");
      return;
    }
  }

  for ( i = 0; i < strip.numPixels(); i++) {
    hsi2rgb( HueArray[i], S, I, rgb);
    c = strip.Color(rgb[0], rgb[1], rgb[2]); //convert rgb value to 4 byte color value
    strip.setPixelColor(i, 0); //turn remaining pixels
    if (digitalRead(NOTIFY_PIN)) {
      Serial.println("Pack() returning due to NOTIFY_PIN");
      return;
    }
  }
  strip.show();
  while (!digitalRead(NOTIFY_PIN)) { //do this forever
    if (digitalRead(NOTIFY_PIN)) {
      Serial.println("Pack() returning due to NOTIFY_PIN");
      return;
    }
    for ( j = 0; j < strip.numPixels() - 4; j++) {

      for ( i = 0; i < j; i++) {
        strip.setPixelColor(i, 0); //turn off leading pixels
        if (digitalRead(NOTIFY_PIN)) {
          Serial.println("Pack() returning due to NOTIFY_PIN");
          return;
        }
      }

      for (i = j; i < j + 4; i++) {
        if (digitalRead(NOTIFY_PIN)) {
          Serial.println("Pack() returning due to NOTIFY_PIN");
          return;
        }
        strip.setPixelColor(i, c);  //turn on pack of pixels
      }

      for ( i = j + 4; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, 0); //turn remaining pixels
        if (digitalRead(NOTIFY_PIN)) {
          Serial.println("Pack() returning due to NOTIFY_PIN");
          return;
        }
      }
      strip.show();
      delay(20);
    } //end j loop

    for ( j = strip.numPixels(); j > 4; j--) {

      for ( i = 0; i < j; i++) {
        strip.setPixelColor(i, 0); //turn off leading pixels
        if (digitalRead(NOTIFY_PIN)) {
          Serial.println("Pack() returning due to NOTIFY_PIN");
          return;
        }
      }

      for (i = j - 4; i < j; i++) {
        if (digitalRead(NOTIFY_PIN)) {
          Serial.println("Pack() returning due to NOTIFY_PIN");
          return;
        }
        hsi2rgb( HueArray[i], S, I, rgb);
        c = strip.Color(rgb[0], rgb[1], rgb[2]); //convert rgb value to 4 byte color value
        strip.setPixelColor(i, c);  //turn on pack of pixels
      }

      for ( i = j; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, 0); //turn remaining pixels
        if (digitalRead(NOTIFY_PIN)) {
          Serial.println("Pack() returning due to NOTIFY_PIN");
          return;
        }
      }
      strip.show();
      delay(20);
    } //end j loop
  }
}

//-------------------------------------------------------------------------------------------------------
int getIRKey() {
  int data[12];
  digitalWrite(LED_PIN, HIGH); //Ok, i'm ready to recieve

  while (pulseIn(IR_PIN, LOW) < 2200) { //Waitfor a start bit
    if (digitalRead(NOTIFY_PIN))return -1;
  }
  data[0] = pulseIn(IR_PIN, LOW); //Stat measuring bits, I only want low pulses
  if (digitalRead(NOTIFY_PIN))return -1;
  data[1] = pulseIn(IR_PIN, LOW);
  if (digitalRead(NOTIFY_PIN))return -1;
  data[2] = pulseIn(IR_PIN, LOW);
  if (digitalRead(NOTIFY_PIN))return -1;
  data[3] = pulseIn(IR_PIN, LOW);
  if (digitalRead(NOTIFY_PIN))return -1;
  data[4] = pulseIn(IR_PIN, LOW);
  if (digitalRead(NOTIFY_PIN))return -1;
  data[5] = pulseIn(IR_PIN, LOW);
  if (digitalRead(NOTIFY_PIN))return -1;
  data[6] = pulseIn(IR_PIN, LOW);
  if (digitalRead(NOTIFY_PIN))return -1;
  data[7] = pulseIn(IR_PIN, LOW);
  if (digitalRead(NOTIFY_PIN))return -1;
  data[8] = pulseIn(IR_PIN, LOW);
  if (digitalRead(NOTIFY_PIN))return -1;
  data[9] = pulseIn(IR_PIN, LOW);
  if (digitalRead(NOTIFY_PIN))return -1;
  data[10] = pulseIn(IR_PIN, LOW);
  if (digitalRead(NOTIFY_PIN))return -1;
  data[11] = pulseIn(IR_PIN, LOW);
  if (digitalRead(NOTIFY_PIN))return -1;
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
    if (digitalRead(NOTIFY_PIN))return -1;
    if (data[i] > 1) {
      Serial.println("getIRKey data[i] > 1.  Returning -1");
      return -1;
    }
  }
  int result = 0;
  int seed = 1;
  for (int i = 0; i < 11; i++) { //Convert bits to integer
    if (digitalRead(NOTIFY_PIN))return -1;
    if (data[i] == 1) {
      result += seed;
    }
    seed = seed * 2;
  }
  return result; //Return key number
}

//-------------------------------------------------------------------------------------------------------
// http://blog.saikoled.com/post/43693602826/why-every-led-light-should-be-using-hsi
// Function example takes H, S, I, and a pointer to the
// returned RGB colorspace converted vector. It should
// be initialized with:
//
// int rgb[3];
//
// in the calling function. After calling hsi2rgb
// the vector rgb will contain red, green, and blue
// calculated values.

void hsi2rgb(float H, float S, float I, int* rgb) {
  int r, g, b;
  H = fmod(H, 360); // cycle H around to 0-360 degrees
  H = 3.14159 * H / (float)180; // Convert to radians.
  S = S > 0 ? (S < 1 ? S : 1) : 0; // clamp S and I to interval [0,1]
  I = I > 0 ? (I < 1 ? I : 1) : 0;

  // Math! Thanks in part to Kyle Miller.
  if (H < 2.09439) {
    r = 255 * I / 3 * (1 + S * cos(H) / cos(1.047196667 - H));
    g = 255 * I / 3 * (1 + S * (1 - cos(H) / cos(1.047196667 - H)));
    b = 255 * I / 3 * (1 - S);
  } else if (H < 4.188787) {
    H = H - 2.09439;
    g = 255 * I / 3 * (1 + S * cos(H) / cos(1.047196667 - H));
    b = 255 * I / 3 * (1 + S * (1 - cos(H) / cos(1.047196667 - H)));
    r = 255 * I / 3 * (1 - S);
  } else {
    H = H - 4.188787;
    b = 255 * I / 3 * (1 + S * cos(H) / cos(1.047196667 - H));
    r = 255 * I / 3 * (1 + S * (1 - cos(H) / cos(1.047196667 - H)));
    g = 255 * I / 3 * (1 - S);
  }
  rgb[0] = r;
  rgb[1] = g;
  rgb[2] = b;
}



