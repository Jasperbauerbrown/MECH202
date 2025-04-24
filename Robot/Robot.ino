#include <SPI.h>
#include <nRF24L01.h>  //Part of the RF24 library
#include <RF24.h>
#include <FastLED.h>

#define LForwardPin 10
#define LBackwardPin 11
#define RForwardPin 8
#define RBackwardPin 9
#define LiftUpPin 4
#define LiftDownPin 5

#define NUM_LEDS 22
#define DATA_PIN 41

RF24 radio(49, 48);  // CE = 49, CSN = 48
const byte address[6] = "00001";

bool debug = false;

struct controlSchema {
  int driveLeft = 0;
  int driveRight = 0;
  int lift = 0;
  bool button = 0;
  int ledMode = 0;
};

int ledMode = -1;

const int radioCheckPeriod = 50;
int lastRec = 0;

CRGB leds[NUM_LEDS];
int lastLEDUpdate = 0;
int ledIndex = 2;
int colorIndex = 0;
bool cyclon = false;

DEFINE_GRADIENT_PALETTE(colors_gp){
  0, 255, 0, 0,    //black
  128, 0, 255, 0,  //red
  224, 0, 0, 255,  //bright yellow
  230, 255, 0, 255,
  255, 255, 0, 255
};  //full whit

CRGBPalette16 myPal = colors_gp;

void setup() {
  // Motor pins as outputs
  pinMode(LForwardPin, OUTPUT);
  pinMode(LBackwardPin, OUTPUT);
  pinMode(RForwardPin, OUTPUT);
  pinMode(RBackwardPin, OUTPUT);
  pinMode(LiftUpPin, OUTPUT);
  pinMode(LiftDownPin, OUTPUT);

  Serial.begin(9600);

  // Initialize NRF24L01 module
  radio.begin();
  radio.setChannel(28);               // Use the same channel as sender
  radio.setPALevel(RF24_PA_HIGH);     // Power level (HIGH for better range), switch to low if experiencing soft resets
  radio.setDataRate(RF24_250KBPS);    // Lower data rate = longer range
  radio.openReadingPipe(0, address);  // Set receiver address
  radio.startListening();             // Set as receiver

  FastLED.addLeds<WS2812, DATA_PIN, GRB>(leds, NUM_LEDS);
  updateLEDs(0);
}

void loop() {
  if (millis() - lastRec >= radioCheckPeriod) {
    if (radio.available()) {
      lastRec = millis();
      controlSchema controls;
      radio.read(&controls, sizeof(controls));  // Read the incoming data

      if (debug) {
        Serial.print("Left motor Speed : ");
        Serial.print(controls.driveLeft);
        Serial.print("  |   Right motor Speed : ");
        Serial.println(controls.driveRight);
        Serial.print("  |   Lift motor Speed : ");
        Serial.println(controls.lift);
      }

      if (controls.driveLeft >= 0) {
        analogWrite(LBackwardPin, 0);
        analogWrite(LForwardPin, controls.driveLeft);
      } else if (controls.driveLeft < 0) {
        analogWrite(LForwardPin, 0);
        analogWrite(LBackwardPin, -controls.driveLeft);
      }
      if (controls.driveRight >= 0) {
        analogWrite(RBackwardPin, 0);
        analogWrite(RForwardPin, controls.driveRight);
      } else if (controls.driveRight < 0) {
        analogWrite(RForwardPin, 0);
        analogWrite(RBackwardPin, -controls.driveRight);
      }
      if (controls.lift >= 0) {
        analogWrite(LiftUpPin, 0);
        analogWrite(LiftDownPin, controls.lift);
      } else if (controls.lift < 0) {
        analogWrite(LiftUpPin, -controls.lift);
        analogWrite(LiftDownPin, 0);
      }
      updateLEDs(controls.ledMode);
    }
  }
  movingLEDRough();
  movingLED();
  pallet();
  rannd();
}

void updateLEDs(int newMode) {
  if (newMode == ledMode) return;
  else ledMode = newMode;

  if (ledMode == 0) {
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = CRGB::Black;
    }
  } else if (ledMode == 1) {
    leds[0] = CRGB::White;
    leds[NUM_LEDS - 1] = CRGB::White;
    for (int i = 1; i < (NUM_LEDS - 1); i++) {
      leds[i] = CRGB(255, 70, 0);
    }
  } else if (ledMode == 2) {
    leds[0] = CRGB(200, 200, 200);
    leds[NUM_LEDS - 1] = CRGB(200, 200, 200);
    leds[1] = CRGB(0, 255, 0);
    leds[NUM_LEDS - 2] = CRGB(0, 255, 0);
    leds[2] = CRGB(255, 255, 255);
    leds[NUM_LEDS - 3] = CRGB(255, 255, 255);
    leds[3] = CRGB(255, 255, 255);
    leds[NUM_LEDS - 4] = CRGB(255, 255, 255);
    for (int i = 4; i < (NUM_LEDS - 4); i++) {
      leds[i] = CRGB(255, 70, 0);
    }
    leds[10] = CRGB(255, 0, 0);
    leds[11] = CRGB(255, 0, 0);
  } else if (ledMode == 3) {
    leds[0] = CRGB(255, 0, 0);
    leds[NUM_LEDS - 1] = CRGB(255, 0, 0);
    leds[1] = CRGB(255, 0, 0);
    leds[NUM_LEDS - 2] = CRGB(255, 0, 0);
  } else if (ledMode == 5) {
    ledIndex = 2;
  }
  FastLED.show();
}

void movingLEDRough() {
  if (ledMode == 3) {
    if (millis() - lastLEDUpdate >= 100) {
      lastLEDUpdate = millis();
      leds[ledIndex] = CRGB(0, 0, 0);
      leds[NUM_LEDS - 1 - ledIndex] = CRGB(0, 0, 0);
      ledIndex++;
      if (ledIndex > NUM_LEDS / 2) ledIndex = 2;
      leds[ledIndex] = CRGB(255, 80, 0);
      leds[NUM_LEDS - 1 - ledIndex] = CRGB(255, 80, 0);
      FastLED.show();
    }
  }
}

void movingLED() {
  if (ledMode == 4) {
    if (millis() - lastLEDUpdate >= 20) {
      lastLEDUpdate = millis();
      fadeall();
      leds[ledIndex] = CRGB(255, 80, 0);
      leds[NUM_LEDS - 1 - ledIndex] = CRGB(255, 80, 0);
      FastLED.show();
      if (cyclon == true) ledIndex++;
      else ledIndex--;
      if (ledIndex <= 2 || ledIndex >= NUM_LEDS - 2) cyclon = !cyclon;
    }
  }
}

void pallet() {
  if (ledMode == 5) {
    if (millis() - lastLEDUpdate >= 10) {
      lastLEDUpdate = millis();
      colorIndex = millis() / 5;
      if (colorIndex > 255) colorIndex = colorIndex - 255;
      for (int i = 0; i < (NUM_LEDS - 1) / 2; ++i) {
        leds[i] = ColorFromPalette(myPal, colorIndex, 255, LINEARBLEND);
        leds[NUM_LEDS - 1 - i] = ColorFromPalette(myPal, colorIndex, 255, LINEARBLEND);
        colorIndex += 14;
        if (colorIndex > 255) colorIndex = colorIndex - 255;
      }
      FastLED.show();
    }
  }
}

void rannd() {
  if (ledMode == 6) {
    if (millis() - lastLEDUpdate >= 200) {
      lastLEDUpdate = millis();
      for (int i = 0; i < NUM_LEDS - 1; ++i) {
        leds[i] = CRGB(int(random(0, 255)), int(random(0, 255)), int(random(0, 255)));
      }
      FastLED.show();
    }
  }
}

void fadeall() {
  for (int i = 2; i < (NUM_LEDS - 2); i++) { leds[i].nscale8(60); }
}
