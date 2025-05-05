#include <SPI.h>
#include <nRF24L01.h>  //part of the RF24 library
#include <RF24.h>

// Define NRF24L01 radio object with CE and CSN pins
RF24 radio(7, 8);  // CE = 7, CSN = 8

// Define the pipe address (must be the same as the receiver)
// This does NOT need to be changed based on your team number.
const byte address[6] = "00001";

int buttonState = 0;
int switchState = 1;
bool buttonDebounce = false;

// Define joystick pins
const int LYPin = A4;
const int LXPin = A5;
const int RYPin = A2;
const int RXPin = A3;
const int buttonPin = 2;
const int switch1Pin = 4;
const int switch2Pin = 5;
const int ledPin = 3;


// This MUST match the struct in your controller code.
struct controlSchema {
  int driveLeft = 0;
  int driveRight = 0;
  int lift = 0;
  bool button = 0;
  int ledMode = 0;
  int switchState = 1;
};

int deadZone = 25;

int xJoystickL = 0;
int yJoystickL = 0;
int xJoystickR = 0;
int yJoystickR = 0;
int ledMode = 0;

void setup() {
  Serial.begin(9600);
  pinMode(buttonPin, INPUT);
  pinMode(switch1Pin, INPUT);
  pinMode(switch2Pin, INPUT);
  pinMode(ledPin, OUTPUT);

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
  controls.button = digitalRead(buttonPin);
  if (controls.button == LOW && buttonDebounce == true) {
    buttonDebounce = false;
  }
  if (digitalRead(switch1Pin) == HIGH) {
    switchState = 0;
  } else if (digitalRead(switch2Pin) == HIGH) {
    switchState = 2;
  } else {
    switchState = 1;
  }
  controls.switchState = switchState;
  if (controls.button == HIGH) {
    digitalWrite(ledPin, HIGH);
  } else {
    digitalWrite(ledPin, LOW);
  }

  xJoystickL = analogRead(LXPin);
  yJoystickL = analogRead(LYPin);
  xJoystickR = analogRead(RXPin);
  yJoystickR = analogRead(RYPin);
  xJoystickL = -map(xJoystickL, 0, 1023, -255, 255);
  yJoystickL = -map(yJoystickL, 0, 1023, -255, 255);
  xJoystickR = -map(xJoystickR, 0, 1023, -255, 255);
  yJoystickR = -map(yJoystickR, 0, 1023, -255, 255);
  if (abs(xJoystickL) < deadZone) xJoystickL = 0;
  if (abs(yJoystickL) < deadZone) yJoystickL = 0;
  if (abs(xJoystickR) < deadZone) xJoystickR = 0;
  if (abs(yJoystickR) < deadZone) yJoystickR = 0;
  if (switchState == 2) {
    controls.lift = yJoystickR;
    controls.driveLeft = constrain(yJoystickL + xJoystickL, -255, 255);
    controls.driveRight = constrain(yJoystickL - xJoystickL, -255, 255);
  } else {
    controls.driveLeft = yJoystickL;
    controls.driveRight = yJoystickR;
  }
  if (controls.button == HIGH && buttonDebounce == false) {
    buttonDebounce = true;
    ledMode++;
    if (ledMode >= 7) ledMode = 0;
  }
  controls.ledMode = ledMode;
  bool success = radio.write(&controls, sizeof(controls));
  printStruct(controls);
  delay(50);
}

void printStruct(controlSchema controls) {
  Serial.print("\n\n\n\n");
  Serial.println("DriveL: " + String(controls.driveLeft) + "   DriveR: " + String(controls.driveRight));
  Serial.println("Button: " + String(controls.button) + "   Switch State: " + String(switchState));
  Serial.println("LEDMode: " + String(controls.ledMode));
}
