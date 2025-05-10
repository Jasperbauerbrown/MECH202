//  MECH202 Group 14 Robot Code (Arduino MEGA)
//  Written by Jasper Bauer-Brown
//  AI Disclaimer: Grok wrote the bit of code that was then chopped up into the custom servo struct
//  Everything else was written by me, including those eyesores of LED functions

#include <SPI.h>
#include <nRF24L01.h>  //Part of the RF24 library
#include <RF24.h>
#include <FastLED.h>

//  Pin Definitions
#define LForwardPin 10
#define LBackwardPin 11
#define RForwardPin 8
#define RBackwardPin 9
#define LiftUpPin 4
#define LiftDownPin 5
#define truckServoPin 3
#define fingerServoPin 2

//  Important LED Constants
#define NUM_LEDS 22
#define DATA_PIN 41

// Custom servo control variables
#define MIN_PULSE_US 500       // 0 degrees
#define MAX_PULSE_US 2500      // 180 degrees
#define SERVO_PERIOD_US 20000  // 20ms = 50Hz

RF24 radio(49, 48);  // CE = 49, CSN = 48
const byte address[6] = "00001";

//  Debug Flag (ik it should be a preprocessor but im lazy)
bool debug = false;

//  Custom Servo Struct
struct servo {
  //  Servo Values
  const int pin;
  int position;
  int servoPulseWidth = 0;
  //  Constructor
  servo(int p, int pos)
    : pin(p), position(pos) {}
  //  Will move servo to whatever position is set to
  void update() {
    position = constrain(position, 0, 180);
    servoPulseWidth = map(position, 0, 180, MIN_PULSE_US, MAX_PULSE_US);
    digitalWrite(pin, HIGH);
    delayMicroseconds(servoPulseWidth);
    digitalWrite(pin, LOW);
    delayMicroseconds(SERVO_PERIOD_US - servoPulseWidth);
  }
  //  Setup function
  void init() {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
    update();
  }
};

//  Custom servo instances
servo truckServo(truckServoPin, 158);
servo fingerServo(fingerServoPin, 90);

//  Control scheme struct
struct controlSchema {
  int driveLeft = 0;
  int driveRight = 0;
  int lift = 0;
  bool servoDeployed = false;
  int ledMode = 0;
  int switchState = 1;
  int pot = 0;
};

//  Current LED Mode
int ledMode = -1;

//  Radio Check interval vars
const int radioCheckPeriod = 50;
int lastRec = 0;

//  Array-representation of LED strip
CRGB leds[NUM_LEDS];
//  All these variables used for timing/progression/activation of light modes
long lastLEDUpdate = 0;
int ledIndex = 2;
int colorIndex = 0;
bool cyclon = false;
unsigned long lastPoliceChangeTime = 0;
unsigned long lastPoliceUpdate = 0;
int speedL = 0, speedR = 0, Lift = 0;
int adaptiveIndexL = 1, adaptiveIndexR = 1;
unsigned long lastLEDUpdateL = 0, lastLEDUpdateR = 0, lastLEDUpdateLift = 0;
bool stoppedLift = false;

//  FastLED rainbow color palette - these are used to make smooth gradients over several LEDs
DEFINE_GRADIENT_PALETTE(colors_gp){
  0, 255, 0, 0,
  128, 0, 255, 0,
  224, 0, 0, 255,
  230, 255, 0, 255,
  255, 255, 0, 255
};

//  Palette for adaptive lights
DEFINE_GRADIENT_PALETTE(forward_gp){
  0, 0, 0, 0,
  38, 0, 0, 0,
  63, 0, 255, 0,
  89, 0, 0, 0,
  165, 0, 0, 0,
  191, 0, 255, 0,
  216, 0, 0, 0,
  255, 0, 0, 0
};

//  Palette for adaptive lights
DEFINE_GRADIENT_PALETTE(reverse_gp){
  0, 0, 0, 0,
  38, 0, 0, 0,
  63, 255, 0, 0,
  89, 0, 0, 0,
  165, 0, 0, 0,
  191, 255, 0, 0,
  216, 0, 0, 0,
  255, 0, 0, 0
};

//  Instantiate palettes - stores them in memory
CRGBPalette16 color_pallet = colors_gp;
CRGBPalette16 forward_pallet = forward_gp;
CRGBPalette16 reverse_pallet = reverse_gp;

void setup() {
  //  Setup motor pins
  pinMode(LForwardPin, OUTPUT);
  pinMode(LBackwardPin, OUTPUT);
  pinMode(RForwardPin, OUTPUT);
  pinMode(RBackwardPin, OUTPUT);
  pinMode(LiftUpPin, OUTPUT);
  pinMode(LiftDownPin, OUTPUT);
  //  Initiate servos
  truckServo.init();
  fingerServo.init();

  Serial.begin(9600);

  //  Radio setup
  radio.begin();
  radio.setChannel(28);
  radio.setPALevel(RF24_PA_HIGH);
  radio.setDataRate(RF24_250KBPS);
  radio.openReadingPipe(0, address);
  radio.startListening();

  //  Tell FastLED to associate array with physical LEDs
  FastLED.addLeds<WS2812, DATA_PIN, GRB>(leds, NUM_LEDS);
  updateLEDs(0);
}

void loop() {
  //  Only want to receive every 50 ms - this is non blocking
  if (millis() - lastRec >= radioCheckPeriod) {
    if (radio.available()) {
      //  Reset timer
      lastRec = millis();
      //  Read Radio
      controlSchema controls;
      radio.read(&controls, sizeof(controls));  // Read the incoming data
      // Helps with debugging if flag is set
      if (debug) {
        Serial.print("Left motor Speed : ");
        Serial.print(controls.driveLeft);
        Serial.print("  |   Right motor Speed : ");
        Serial.println(controls.driveRight);
        Serial.print("  |   Lift motor Speed : ");
        Serial.println(controls.lift);
      }

      //  Motor controls can be directly applied to motors
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

      //  Potentiometer value is pre-processed on controller
      //  But throw another limit on it just in case, dont want to break it
      if (controls.pot >= 70) {
        truckServo.position = controls.pot;
      }
      
      //  When this goes true the secret weapon deploys
      if (controls.servoDeployed == true) {
        fingerServo.position = 180;
      }
      else if (controls.servoDeployed == false && fingerServo.position != 0) {
        fingerServo.position = 90;
      }
      //  Check LED mode with every new packet
      updateLEDs(controls.ledMode);

      //  These vars are set with each packet in case we are in adaptive light mode
      Lift = controls.lift;
      speedL = controls.driveLeft;
      speedR = controls.driveRight;
    }
  }
  //  Need to make sure servos get their PWM signals each loop
  truckServo.update();
  fingerServo.update();
  //  Non-static led modes need to be iterated each loop
  //  In reality most of these functions escape immediately, at most 1 willl run
  adaptiveLED();
  movingRoundLED();
  movingSymLED();
  rainbowGradient();
  police();
}

//  This function will set the current LED mode
//  This also will make any static color assignments for that light mode
//  (also resets some critical vars for non-static modes)
void updateLEDs(int newMode) {
  if (newMode == ledMode) return;
  else ledMode = newMode;

  if (ledMode == 0) {
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = CRGB::Black;
    }
  } else if (ledMode == 1) {
    colorIndex = 0;
  } else if (ledMode == 2) {
    leds[0] = CRGB(255, 255, 255);
    leds[NUM_LEDS - 1] = CRGB(255, 255, 255);
    leds[1] = CRGB(0, 255, 0);
    leds[NUM_LEDS - 2] = CRGB(0, 255, 0);
    leds[2] = CRGB(255, 255, 255);
    leds[NUM_LEDS - 3] = CRGB(255, 255, 255);
    for (int i = 3; i < (NUM_LEDS - 3); i++) {
      leds[i] = CRGB(255, 70, 0);
    }
    leds[10] = CRGB(255, 0, 0);
    leds[11] = CRGB(255, 0, 0);
  } else if (ledMode == 3) {
    leds[0] = CRGB(255, 0, 0);
    leds[NUM_LEDS - 1] = CRGB(255, 0, 0);
    leds[1] = CRGB(255, 0, 0);
    leds[NUM_LEDS - 2] = CRGB(255, 0, 0);
  } else if (ledMode == 4) {
    ledIndex = 3;
  } else if (ledMode == 5) {
    ledIndex = 2;
  } else if (ledMode == 6) {
    colorIndex = 0;
    leds[0] = CRGB(255, 255, 255);
    leds[NUM_LEDS - 1] = CRGB(255, 255, 255);
  }
  //  Called to update physical LEDs to contents of LED array
  FastLED.show();
}

//  This code is an eyesore - basically we want the lights on each
//  side of the robot to match the speed and direction of the respective drive motor.
//  We leverage FastLEDs gradient functionality to make it looks smooth.
//  We also want to blink the lift lights if the lift is moving, and turn them white if its not.
void adaptiveLED() {
  if (ledMode == 1) {
    if (abs(speedL) != 0 && millis() - lastLEDUpdateL >= 20) {
      lastLEDUpdateL = millis();
      adaptiveIndexL = (adaptiveIndexL + (1 + (abs(speedL) / 35)) * (-speedL / abs(speedL))) % 256;
      CRGBPalette16 *palletL = &forward_pallet;
      if (speedL < 0) {
        palletL = &reverse_pallet;
      }
      int subindex = adaptiveIndexL;
      for (int i = 3; i < 11; i++) {
        subindex += 14;
        leds[i] = ColorFromPalette(*palletL, subindex, 255, LINEARBLEND);
        if (subindex > 255) subindex = 0;
      }
      FastLED.show();
    }
    if (abs(speedR) != 0 && millis() - lastLEDUpdateR >= 20) {
      lastLEDUpdateR = millis();
      adaptiveIndexR = (adaptiveIndexR + (1 + (abs(speedR) / 35)) * (speedR / abs(speedR))) % 256;
      CRGBPalette16 *palletR = &forward_pallet;
      if (speedR < 0) {
        palletR = &reverse_pallet;
      }
      int subindex = adaptiveIndexR;
      for (int i = 11; i < 19; i++) {
        subindex += 14;
        leds[i] = ColorFromPalette(*palletR, subindex, 255, LINEARBLEND);
        if (subindex > 255) subindex = 0;
      }
      FastLED.show();
    }
    if (Lift == 0 && stoppedLift == false) {
      stoppedLift == true;
      leds[0] = CRGB(255, 255, 255);
      leds[1] = CRGB(255, 255, 255);
      leds[2] = CRGB(255, 255, 255);
      leds[19] = CRGB(255, 255, 255);
      leds[20] = CRGB(255, 255, 255);
      leds[21] = CRGB(255, 255, 255);
      FastLED.show();
    } else if (millis() - lastLEDUpdateLift >= (600 - abs(Lift))) {
      stoppedLift == false;
      lastLEDUpdateLift = millis();
      if (leds[0] == CRGB(0, 0, 0)) {
        CRGB color = CRGB(0, 0, 0);
        if (Lift > 0) color = CRGB(0, 255, 0);
        else color = CRGB(255, 0, 0);
        leds[0] = color;
        leds[1] = color;
        leds[2] = color;
        leds[19] = color;
        leds[20] = color;
        leds[21] = color;
      } else {
        leds[0] = CRGB(0, 0, 0);
        leds[1] = CRGB(0, 0, 0);
        leds[2] = CRGB(0, 0, 0);
        leds[19] = CRGB(0, 0, 0);
        leds[20] = CRGB(0, 0, 0);
        leds[21] = CRGB(0, 0, 0);
      }
      FastLED.show();
    }
  }
}

//  This mode makes a single orange dot circle around the bottom lights,
//  looks like one of those hazard spinny lights
void movingRoundLED() {
  if (ledMode == 3) {
    if (millis() - lastLEDUpdate >= 25) {
      lastLEDUpdate = millis();
      fadeall();
      leds[ledIndex] = CRGB(255, 80, 0);
      FastLED.show();
      ledIndex++;
      if (ledIndex >= NUM_LEDS - 2) ledIndex = 2;
    }
  }
}

//  This mode makes symmetrical dots that move forward and backward
//  and "bounce" at the edges across the bottom lights.
//  I think this is called a cyclon?
void movingSymLED() {
  if (ledMode == 4) {
    if (millis() - lastLEDUpdate >= 60) {
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

//  This is by far the smoothest looking mode.
//  Again use FastLED gradient, but we have a big, spread out gradient across all 22 LEDs
//  so we can go 1 or 2 indexes at a time and it will look really smooth
void rainbowGradient() {
  if (ledMode == 5) {
    if (millis() - lastLEDUpdate >= 10) {
      lastLEDUpdate = millis();
      colorIndex = (millis() / 5) % 256;
      for (int i = 0; i < NUM_LEDS / 2; ++i) {
        leds[i] = ColorFromPalette(color_pallet, colorIndex, 255, LINEARBLEND);
        leds[NUM_LEDS - 1 - i] = ColorFromPalette(color_pallet, colorIndex, 255, LINEARBLEND);
        colorIndex += 14;
        if (colorIndex > 255) colorIndex = colorIndex - 255;
      }
      FastLED.show();
    }
  }
}

//  This one got a bit out of hand - police/emergency lights
//  that auto cycles through three different patterns, sort of like
//  how actually emergency lights will change patterns. I reused a lot of
//  other light mode vars for this, so names might not make sense.
void police() {
  if (ledMode == 6) {
    if (millis() - lastLEDUpdate >= 50) {
      lastLEDUpdate = millis();
      if (colorIndex == 0 && millis() - lastPoliceUpdate > 250) {
        lastPoliceUpdate = millis();
        if (cyclon == true) {
          for (int i = 1; i < NUM_LEDS / 2; i++) {
            leds[i] = CRGB(255, 0, 0);
            leds[NUM_LEDS - 1 - i] = CRGB(0, 0, 0);
          }
          cyclon = !cyclon;
        } else if (cyclon == false) {
          for (int i = 1; i < NUM_LEDS / 2; i++) {
            leds[i] = CRGB(0, 0, 0);
            leds[NUM_LEDS - 1 - i] = CRGB(0, 0, 255);
          }
          cyclon = !cyclon;
        }
      } else if (colorIndex == 1 && millis() - lastPoliceUpdate > 150) {
        lastPoliceUpdate = millis();
        if (cyclon == true) {
          for (int i = 1; i < (NUM_LEDS - 10) / 2; i++) {
            leds[i] = CRGB(255, 0, 0);
            leds[NUM_LEDS - 1 - i] = CRGB(0, 0, 255);
          }
          for (int i = 6; i < (NUM_LEDS) / 2; i++) {
            leds[i] = CRGB(0, 0, 0);
            leds[NUM_LEDS - 2 - i] = CRGB(0, 0, 0);
          }
          cyclon = !cyclon;
        } else if (cyclon == false) {
          for (int i = 7; i < (NUM_LEDS) / 2; i++) {
            leds[i] = CRGB(0, 0, 255);
            leds[NUM_LEDS - 1 - i] = CRGB(255, 0, 0);
          }
          for (int i = 1; i < (NUM_LEDS - 8) / 2; i++) {
            leds[i] = CRGB(0, 0, 0);
            leds[NUM_LEDS - 1 - i] = CRGB(0, 0, 0);
          }
          cyclon = !cyclon;
        }
      } else if (colorIndex == 2 && millis() - lastPoliceUpdate > 50) {
        lastPoliceUpdate = millis();
        CRGB color = CRGB(0, 0, 0);
        for (int i = 1; i < NUM_LEDS - 1; i++) {
          int rand = random(0, 3);
          if (rand == 0) {
            color = CRGB(255, 0, 0);
          } else if (rand == 1) {
            color = CRGB(0, 0, 255);
          } else if (rand == 2) {
            color = CRGB(0, 0, 0);
          }
          leds[i] = color;
        }
      }
      if (millis() - lastPoliceChangeTime >= 9000) {
        lastPoliceChangeTime = millis();
        colorIndex++;
        if (colorIndex > 2) colorIndex = 0;
      }
      FastLED.show();
    }
  }
}

//  These functions are usefull to slowly fade out blocks of LEDs.
//  All, left and right under carraige
void fadeall() {
  for (int i = 2; i < (NUM_LEDS - 2); i++) { leds[i].nscale8(60); }
}
void fadeallLeft() {
  for (int i = 2; i < (NUM_LEDS) / 2; i++) { leds[i].nscale8(80); }
}
void fadeallRight() {
  for (int i = 11; i < (NUM_LEDS - 2); i++) { leds[i].nscale8(80); }
}
