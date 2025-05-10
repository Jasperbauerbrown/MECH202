#include <SPI.h>
#include <nRF24L01.h>  //part of the RF24 library
#include <RF24.h>

const bool debug = false;

// Define NRF24L01 radio object with CE and CSN pins
RF24 radio(7, 8);  // CE = 7, CSN = 8

// Define the pipe address (must be the same as the receiver)
// This does NOT need to be changed based on your team number.
const byte address[6] = "00001";

bool servoDeployed = false;
int littleButtonState = 0;
int switchState = 1;
bool armSwitchState = false;
bool littleButtonDebounce = false;
bool bigButtonDebounce = false;
int pot = 0;

// Define joystick pins
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


// This MUST match the struct in your controller code.
struct controlSchema {
  int driveLeft = 0;
  int driveRight = 0;
  int lift = 0;
  bool servoDeployed = false;
  int ledMode = 0;
  int switchState = 1;
  int pot = 0;
};

int deadZone = 25;

int xJoystickL = 0;
int yJoystickL = 0;
int xJoystickR = 0;
int yJoystickR = 0;
int ledMode = 0;

void setup() {
  Serial.begin(9600);
  pinMode(littleButtonPin, INPUT);
  pinMode(bigButtonPin, INPUT);
  pinMode(switch1Pin, INPUT_PULLUP);
  pinMode(switch2Pin, INPUT_PULLUP);
  pinMode(armSwitchPin, INPUT);

  // Initialize NRF24L01 module
  radio.begin();
  radio.setChannel(28);             // CHANGE THIS TO YOUR TEAM # * 2!!! Team 38 should use channel 76
  radio.setPALevel(RF24_PA_HIGH);   // Power level (HIGH for better range), switch to low if experiencing soft resets
  radio.setDataRate(RF24_250KBPS);  // Lower data rate = longer range
  radio.openWritingPipe(address);   // Set transmitter address
  radio.stopListening();            // Set as transmitter
}

void loop() {
  controlSchema controls;
  littleButtonState = digitalRead(littleButtonPin);
  if (littleButtonState == LOW && littleButtonDebounce == true) {
    littleButtonDebounce = false;
  }
  if (digitalRead(switch1Pin) == LOW) {
    switchState = 0;
  } else if (digitalRead(switch2Pin) == LOW) {
    switchState = 2;
  } else {
    switchState = 1;
  }
  armSwitchState = digitalRead(armSwitchPin);
  if (armSwitchState == true && digitalRead(bigButtonPin) == HIGH) {
    servoDeployed = true;
  }
  else if (armSwitchState == false) servoDeployed = false;

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
  if (switchState == 2 || switchState == 0) {
    controls.lift = yJoystickR;
    controls.driveLeft = constrain(yJoystickL + xJoystickL, -255, 255);
    controls.driveRight = constrain(yJoystickL - xJoystickL, -255, 255);
  } else {
    controls.driveLeft = yJoystickL;
    controls.driveRight = yJoystickR;
  }
  if (littleButtonState == HIGH && littleButtonDebounce == false) {
    littleButtonDebounce = true;
    ledMode++;
    if (ledMode >= 7) ledMode = 0;
  }
  controls.switchState = switchState;
  controls.ledMode = ledMode;
  int newPot = map(analogRead(potPin), 0, 1023, 70, 180);
  if (abs(newPot - pot) > 5) {
    pot = newPot;
    controls.pot = newPot;
  }
  else controls.pot = pot;
  controls.servoDeployed = servoDeployed;
  bool success = radio.write(&controls, sizeof(controls));
  Serial.println(switchState);

  //if (debug) printStruct(controls);
  delay(50);
}

// void printStruct(controlSchema controls) {
//   Serial.print("\n\n\n\n");
//   Serial.println("DriveL: " + String(controls.driveLeft) + "   DriveR: " + String(controls.driveRight));
//   Serial.println("Button: " + String(controls.button) + "   Switch State: " + String(switchState));
//   Serial.println("LEDMode: " + String(controls.ledMode));
// }
