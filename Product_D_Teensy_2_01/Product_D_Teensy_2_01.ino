#include "Adafruit_NeoTrellis.h"
//trellis details
Adafruit_NeoTrellis trellis;
int trellisButtons[16] = { LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW };
int ledPad = 1;
int ledState[16] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };

#define INT_PIN 27

//define a callback for key presses
TrellisCallback blink(keyEvent evt) {
  if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING) {
    trellisButtons[15 - evt.bit.NUM] = HIGH;
    //turn on MIDI notes
    // usbMIDI.sendNoteOn((60 + keyIncrease + realNoteArray[15 - evt.bit.NUM]), 127, 1);
    Serial.print("ON IS");
    Serial.println(evt.bit.NUM);
  } else if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_FALLING) {
    trellisButtons[15 - evt.bit.NUM] = LOW;
    //turn off MIDI notes
    // usbMIDI.sendNoteOff((60 + keyIncrease + realNoteArray[15 - evt.bit.NUM]), 127, 1);
    Serial.print("OFF IS");
    Serial.println(evt.bit.NUM);
  }
  // Turn on/off the neopixels!
  // trellis.pixels.show();
  return 0;
}

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);
  pinMode(INT_PIN, INPUT);

  if (!trellis.begin()) {
    Serial.println("could not start trellis");
    while (1) delay(1);
  } else {
    Serial.println("trellis started");
  }

  //activate all keys and set callbacks
  for (int i = 0; i < NEO_TRELLIS_NUM_KEYS; i++) {
    trellis.activateKey(i, SEESAW_KEYPAD_EDGE_RISING);
    trellis.activateKey(i, SEESAW_KEYPAD_EDGE_FALLING);
    trellis.registerCallback(i, blink);
  }

  //do a little animation to show we're on
  for (uint16_t i = 0; i < trellis.pixels.numPixels(); i++) {
    trellis.pixels.setPixelColor(i, 255, 255, 255);
    trellis.pixels.show();
    delay(50);
  }
  for (uint16_t i = 0; i < trellis.pixels.numPixels(); i++) {
    trellis.pixels.setPixelColor(i, 0x000000);
    trellis.pixels.show();
    delay(50);
  }
}

void loop() {
  if (!digitalRead(INT_PIN)) {
    trellis.read(false);
    checkLed();
  }
  if (Serial.available()) {
    checkByte();
    delay(2);
    trellis.pixels.show();
  }
}

// void onButtonPress(int _buttonNum) {
//   Serial.println(_buttonNum);
//   Serial1.write(_buttonNum);
// }

void checkLed() {  //eliminates LED latency
  for (int i = 0; i < 16; i++) {
    if (trellisButtons[15 - i] == HIGH) {
      switch (ledPad) {
        case 1:
          trellis.pixels.setPixelColor(i, 255, 255, 255);  //on rising
          usbMIDI.sendNoteOn(67 + i, 127, 21);
          ledPad = 2;
          break;
        case 2:
          trellis.pixels.setPixelColor(i, 0);  //off falling
          usbMIDI.sendNoteOff(67 + i, 0, 21);
          ledPad = 1;
          break;
      }
    }
  }
  trellis.pixels.show();
}

void checkByte() {
  int incomingByte = Serial.parseInt() - 1;
  if (Serial.read() == '\n') {
    Serial.println(incomingByte);
    switch (ledState[incomingByte]) {
      case 1:
        trellis.pixels.setPixelColor(incomingByte, 255, 255, 255);
        ledState[incomingByte] = 2;
        break;
      case 2:
        trellis.pixels.setPixelColor(incomingByte, 0);
        ledState[incomingByte] = 1;
        break;
    }
  }
}
