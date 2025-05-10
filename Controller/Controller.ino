//  MECH202 Group 14 Controller Code (Arduino UNO)
//  Written by Jasper Bauer-Brown
//  Disclaimer: Code was built of example controller code

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

//  Debug flag (depracated, but it sounds so professional to have a debug flag)
const bool debug = false;

//  Radio objects
RF24 radio(7, 8);  // CE = 7, CSN = 8
const byte address[6] = "00001";

//  Pin definitions (why arent these #defines idk but I dont want to change them)
const int RYPin = A3;
const int RXPin = A2;
const int LYPin = A1;
const int LXPin = A0;
const int potPin = A4;
const int bigButtonPin = 2;
const int littleButtonPin = 3;
const int switch1Pin = 6;
const int switch2Pin = 5;
const int armSwitchPin = 9;

//  Control struct
struct controlSchema {
  int driveLeft = 0;
  int driveRight = 0;
  int lift = 0;
  bool servoDeployed = false;
  int ledMode = 0;
  int switchState = 1;
  int pot = 0;
};

//  These vars hold states of buttons/switches and previous values.
//  Helpfull when doing logic so we arent polling pins as much
bool servoDeployed = false;
int littleButtonState = 0;
int switchState = 1;
bool armSwitchState = false;
bool littleButtonDebounce = false;
bool bigButtonDebounce = false;
int pot = 0;
//  Joystick positions
int xJoystickL = 0;
int yJoystickL = 0;
int xJoystickR = 0;
int yJoystickR = 0;
int ledMode = 0;

//  Deadzone for joysticks
int deadZone = 25;

void setup() {
  Serial.begin(9600);
  //  Setup input pins - both buttons and switches are high with
  //  pull down circuitry. 3 position switch (2 pins) is low, 
  //  needs to be pulled high internally
  pinMode(littleButtonPin, INPUT);
  pinMode(bigButtonPin, INPUT);
  pinMode(switch1Pin, INPUT_PULLUP);
  pinMode(switch2Pin, INPUT_PULLUP);
  pinMode(armSwitchPin, INPUT);

  // Initialize radio
  radio.begin();
  radio.setChannel(28);
  radio.setPALevel(RF24_PA_HIGH);
  radio.setDataRate(RF24_250KBPS);
  radio.openWritingPipe(address);
  radio.stopListening();
}

void loop() {
  controlSchema controls;
  //  Read little button (led control) and reset debouncer if its low
  littleButtonState = digitalRead(littleButtonPin);
  if (littleButtonState == LOW && littleButtonDebounce == true) {
    littleButtonDebounce = false;
  }
  //  Read in poth pins from 3 position switch, determine position
  if (digitalRead(switch1Pin) == LOW) {
    switchState = 0;
  } else if (digitalRead(switch2Pin) == LOW) {
    switchState = 2;
  } else {
    switchState = 1;
  }
  //  Read in arm switch state
  armSwitchState = digitalRead(armSwitchPin);
  //  We only want to set servoDepolyed if button is pressed AND switch is armred
  if (armSwitchState == true && digitalRead(bigButtonPin) == HIGH) {
    servoDeployed = true;
  }
  else if (armSwitchState == false) servoDeployed = false;
  //  Read in, map, and deadzone joysticks.
  //  Prolly should deadzone before mapping but wtv
  //  Want to spit out -255 to 255, for direct to motors
  xJoystickL = analogRead(LXPin);
  yJoystickL = analogRead(LYPin);
  xJoystickR = analogRead(RXPin);
  yJoystickR = analogRead(RYPin);
  xJoystickL = map(xJoystickL, 0, 1023, -255, 255);
  yJoystickL = map(yJoystickL, 0, 1023, -255, 255);
  xJoystickR = map(xJoystickR, 0, 1023, -255, 255);
  yJoystickR = map(yJoystickR, 0, 1023, -255, 255);
  if (abs(xJoystickL) < deadZone) xJoystickL = 0;
  if (abs(yJoystickL) < deadZone) yJoystickL = 0;
  if (abs(xJoystickR) < deadZone) xJoystickR = 0;
  if (abs(yJoystickR) < deadZone) yJoystickR = 0;
  //  If switch is up or down, drive motors from left stick,
  //  lift from right. Otherwise tank
  if (switchState == 2 || switchState == 0) {
    controls.lift = yJoystickR;
    controls.driveLeft = constrain(yJoystickL + xJoystickL, -255, 255);
    controls.driveRight = constrain(yJoystickL - xJoystickL, -255, 255);
  } else {
    controls.driveLeft = yJoystickL;
    controls.driveRight = yJoystickR;
  }
  //  If pressed and debounce is good, increment light mode
  if (littleButtonState == HIGH && littleButtonDebounce == false) {
    littleButtonDebounce = true;
    ledMode++;
    if (ledMode >= 7) ledMode = 0;
  }
  //  We really don't need to pass switchState, 
  //  I don't think we use it for anything on robot end
  controls.switchState = switchState;
  controls.ledMode = ledMode;
  //  Map and deadzone potentiometer - this gets it to the position
  //  range we need for the truck servo
  int newPot = map(analogRead(potPin), 0, 1023, 70, 180);
  if (abs(newPot - pot) > 2) {
    pot = newPot;
    controls.pot = newPot;
  }
  else controls.pot = pot;
  //  This is the "secret weapon" servo
  controls.servoDeployed = servoDeployed;
  bool success = radio.write(&controls, sizeof(controls));
  //if (debug) printStruct(controls);
  //  Im okay using a blocking delay here since theres nothing outside this loop
  delay(50);
}

//  I need to update this for the current struct
// void printStruct(controlSchema controls) {
//   Serial.print("\n\n\n\n");
//   Serial.println("DriveL: " + String(controls.driveLeft) + "   DriveR: " + String(controls.driveRight));
//   Serial.println("Button: " + String(controls.button) + "   Switch State: " + String(switchState));
//   Serial.println("LEDMode: " + String(controls.ledMode));
// }
