#include <Encoder.h>
#include <StevesAwesomeButton.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Adafruit_NeoTrellis.h"
#include <set>
int partyNum = 0;
std::set<int> usedNumbers;  // Declare a Set to store used numbers

//<<<Encoder global variable section>>>

Encoder encoderArray[] = {
  { 5, 6 },    //slice start
  { 7, 8 },    //slice end
  { 11, 12 },  //time
  { 28, 29 },  //pitch
  { 34, 35 },  //channel volume
  { 20, 21 },  //fx param 1
  { 14, 15 },  //fx param 2
  { 41, 13 },  //fx param 3
  { 39, 40 },  //fx param 4
  { 3, 4 },    //channel select sequencer
  { 22, 23 },  //channel select fx section
  { 37, 38 },  // fx select knob
};
//size of the array constant variable
const size_t numEncoders = sizeof(encoderArray) / sizeof(encoderArray[0]);
//midi control for left click encoder
const int midiNotesEncL[9] = { 86, 87, 88, 89, 90, 91, 92, 93, 94 };
// midi notes for adsr L
const int midiNotesADSRL[4] = { 104, 105, 106, 107 };
//midi control for right click encoder
const int midiNotesEncR[9] = { 95, 96, 97, 98, 99, 100, 101, 102, 103 };
//midi notes for adsr R
const int midiNotesADSRR[4] = { 108, 109, 110, 111 };
// Encoder count variables
int oldPositions[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
int count = 1;
int delayCount = 1;
int delayCountLoop = 7;
bool delayCountToggle = false;
bool fxToADSR = false;

//<<<Button global variable section>>>

StevesAwesomeButton button1(2, 0, INPUT_PULLUP);   //tap tempo
StevesAwesomeButton button2(30, 1, INPUT_PULLUP);  //random step
StevesAwesomeButton button3(31, 2, INPUT_PULLUP);  //clear grid
StevesAwesomeButton button4(32, 3, INPUT_PULLUP);  //play sequencer
StevesAwesomeButton button5(9, 4, INPUT_PULLUP);   //play sample
StevesAwesomeButton button6(10, 5, INPUT_PULLUP);  //crop sample selection
StevesAwesomeButton button7(33, 6, INPUT_PULLUP);  //decouple
StevesAwesomeButton button8(36, 7, INPUT_PULLUP);  //encoder switch looper to sequencer fx
//button call array
StevesAwesomeButton* buttons[8] = { &button1, &button2, &button3, &button4, &button5, &button6, &button7, &button8 };
const int totalButtons = 8;

//<<<OLED Global Variable section>>>

#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library.
#define OLED_RESET_1 -1        // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS_1 0x3C  ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
#define SCREEN_ADDRESS_2 0x3C  ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display1(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET_1);
Adafruit_SSD1306 display2(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire1, OLED_RESET_1);

//<<<NeoTrellis variable section>>>
int gridChannel = 21;
const int chTogglePin = 1;
bool previousToggleState = HIGH;
const int gridMIDINotes[16] = { 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82 };
//create a matrix of trellis panels
const int numRows = 4;
const int numCols = 20;
#define Y_DIM 4   //number of rows of key
#define X_DIM 20  //number of columns of keys
#define Z_DIM 16
// Define an array to keep track of LED states
bool ledStates[Y_DIM * X_DIM] = { false };
// Define an array to keep track of LED states excluding columns 17-20
bool ledStatesSubset[Y_DIM * Z_DIM] = { false };
#define Y_DIM_LIVEGRID 4   // Number of rows for Live.Grid
#define X_DIM_LIVEGRID 20  // Number of columns for Live.Grid
// Define arrays to keep track of LED states
bool ledStatesLiveGrid[Y_DIM_LIVEGRID * X_DIM_LIVEGRID] = { false };  // For Live.Grid
// Define an array for mapping excluded button numbers to MIDI notes and channels
const int excludedButtonMapping[][3] = {
  // {Button number, MIDI note, MIDI channel}
  { 16, 63, 7 },
  { 17, 64, 7 },
  { 18, 65, 7 },
  { 19, 66, 7 },
  { 36, 63, 8 },
  { 37, 64, 8 },
  { 38, 65, 8 },
  { 39, 66, 8 },
  { 56, 63, 9 },
  { 57, 64, 9 },
  { 58, 65, 9 },
  { 59, 66, 9 },
  { 76, 63, 10 },
  { 77, 64, 10 },
  { 78, 65, 10 },
  { 79, 66, 10 },
};
bool isExcludedButton = false;
Adafruit_NeoTrellis t_array[Y_DIM / 4][X_DIM / 4] = {

  { Adafruit_NeoTrellis(0x2E), Adafruit_NeoTrellis(0x2F), Adafruit_NeoTrellis(0x30), Adafruit_NeoTrellis(0x31), Adafruit_NeoTrellis(0x32) }

};

//pass this matrix to the multitrellis object
Adafruit_MultiTrellis trellis((Adafruit_NeoTrellis*)t_array, Y_DIM / 4, X_DIM / 4);

// Input a value 0 to 255 to get a color value.
// The colors are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if (WheelPos < 85) {
    return seesaw_NeoPixel::Color(20, 20, 20);
  } else if (WheelPos < 170) {
    WheelPos -= 85;
    return seesaw_NeoPixel::Color(20, 20, 20);
  } else {
    WheelPos -= 170;
    return seesaw_NeoPixel::Color(20, 20, 20);
  }
  return 0;
}

TrellisCallback blink(keyEvent evt) {
  if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING) {
    int row = evt.bit.NUM / 20;
    int column = evt.bit.NUM % 20 + 1;
    int midiNote;
    int midiChannel;

    // Check if the button number is in the array of excluded button mappings
    for (size_t i = 0; i < sizeof(excludedButtonMapping) / sizeof(excludedButtonMapping[0]); i++) {
      if (evt.bit.NUM == excludedButtonMapping[i][0]) {
        isExcludedButton = true;
        // Set specific values for midiNote and midiChannel
        midiNote = excludedButtonMapping[i][1];
        midiChannel = excludedButtonMapping[i][2];
        break;
      }
    }

    if (!isExcludedButton) {
      // Handle regular buttons
      // Calculate midiNote based on the column
      midiNote = gridMIDINotes[column - 1];
      if (digitalRead(chTogglePin) == HIGH) {
        midiChannel = 26 - row;
      } else {
        midiChannel = 22 - row;
      }
    }

    usbMIDI.sendNoteOn(midiNote, 127, midiChannel);
    if (digitalRead(chTogglePin) == HIGH) {
      ledStates[evt.bit.NUM] = !ledStates[evt.bit.NUM];
    } else if (digitalRead(chTogglePin) == LOW) {
      ledStatesLiveGrid[evt.bit.NUM] = !ledStatesLiveGrid[evt.bit.NUM];
    }

    if (digitalRead(chTogglePin) == HIGH && ledStates[evt.bit.NUM] == HIGH) {
      trellis.setPixelColor(evt.bit.NUM, seesaw_NeoPixel::Color(20, 20, 20));  // on rising
    } else if (digitalRead(chTogglePin) == LOW && ledStatesLiveGrid[evt.bit.NUM] == HIGH) {
      trellis.setPixelColor(evt.bit.NUM, seesaw_NeoPixel::Color(20, 20, 20));  // on rising
    } else {
      trellis.setPixelColor(evt.bit.NUM, seesaw_NeoPixel::Color(0, 0, 0));  // off rising
    }

    trellis.show();

  } else if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_FALLING) {
    if (isExcludedButton) {
      // Handle falling edge for excluded buttons
      trellis.setPixelColor(evt.bit.NUM, seesaw_NeoPixel::Color(0, 0, 0));  // off falling
      trellis.show();

      // Set the LED state to LOW for the excluded button
      if (digitalRead(chTogglePin) == HIGH) {
        ledStates[evt.bit.NUM] = LOW;
      } else {
        ledStatesLiveGrid[evt.bit.NUM] = LOW;
      }
      isExcludedButton = false;
    }
  }
  return 0;
}

//<<<Pots section for Teensy 1>>>
// pot arrays for the Teensy 1 Looper
int potT1[2] = { A11, A12 };
int potValueT1[2] = { 0, 0 };
int ccValueT1[2] = { 0, 0 };
int lastCCValueT1[2] = { 0, 0 };
// Tempo Pot
int potTempo = A10;
int potValueTempo = 0;
int ccValueTempo = 0;
int lastCCValueTempo = 0;

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);  //teensy 2 communications over this channel.
  pinMode(chTogglePin, INPUT_PULLUP);
  randomSeed(analogRead(0));  // Seed the random number generator with analog input
  Serial.println("Basic Encoder Test:");
  for (int b = 0; b < totalButtons; b++) {
    buttons[b]->pressHandler(onButtonPress);
  }
  if (!trellis.begin()) {
    Serial.println("failed to begin trellis");
    while (1) delay(1);
  }

  /* the array can be addressed as x,y or with the key number */
  for (int i = 0; i < Y_DIM * X_DIM; i++) {
    trellis.setPixelColor(i, seesaw_NeoPixel::Color(20, 20, 20));  //on rising
    //trellis.setPixelColor(i, Wheel(map(i, 0, X_DIM*Y_DIM, 0, 255))); //addressed with keynum
    trellis.show();
    delay(25);
  }

  for (int y = 0; y < Y_DIM; y++) {
    for (int x = 0; x < X_DIM; x++) {
      //activate rising and falling edges on all keys
      trellis.activateKey(x, y, SEESAW_KEYPAD_EDGE_RISING, true);
      trellis.activateKey(x, y, SEESAW_KEYPAD_EDGE_FALLING, true);
      trellis.registerCallback(x, y, blink);
      trellis.setPixelColor(x, y, 0x000000);  //addressed with x,y
      trellis.show();                         //show all LEDs
      delay(25);
    }
  }
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  // Initialize the first display
  if (!display1.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS_1)) {
    Serial.println(F("SSD1306 allocation failed for Display 1"));
    for (;;)
      ;  // Don't proceed, loop forever
  }
  if (!display2.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS_2)) {
    Serial.println(F("SSD1306 allocation failed for Display 2"));
    for (;;)
      ;  // Don't proceed, loop forever
  }
  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display1.display();
  delay(1000);  // Pause for 2 seconds

  // Clear the buffer
  display1.clearDisplay();

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display2.display();
  delay(1000);  // Pause for 2 seconds

  // Clear the buffer
  display2.clearDisplay();
}


void loop() {
  for (int b = 0; b < totalButtons; b++) {
    buttons[b]->process();
  }
  checkEncoder();
  allPotLoops();
  valuesT1();
  channelOLED1();
  channelOLED2();
  trellis.read();
  chanToggle();
}

void onButtonPress(int _buttonNum) {
  if (_buttonNum == 0) {  //tap tempo
    usbMIDI.sendNoteOn(112, 127, 1);
  }
  if (_buttonNum == 1) {  //random step
    generateAndProcessMainGrid();
    generateAndProcessLiveGrid();
    if (digitalRead(chTogglePin) == HIGH) {
      updateNeoTrellisLEDs();
    } else {
      updateNeoTrellisLEDsLiveGrid();
    }
  }
  if (_buttonNum == 2) {  //clear grid
    usbMIDI.sendNoteOn(84, 127, 1);
    clearAllLEDs();
  }
  if (_buttonNum == 3) {  //play sequencer
    usbMIDI.sendNoteOn(85, 127, 1);
  }
  if (_buttonNum == 4) {  //play sample
    usbMIDI.sendNoteOn(61, 127, count);
  }
  if (_buttonNum == 5) {  //crop sample selection
    usbMIDI.sendNoteOn(62, 127, count);
  }
  if (_buttonNum == 6) {  //decouple time and pitch
    usbMIDI.sendNoteOn(60, 127, count);
  }
  if (_buttonNum == 7) {  //encoder switch fx sequencer to looper
    delayCountToggle = !delayCountToggle;
  }
}

void checkEncoder() {
  for (int i = 0; i < 12; i++) {
    int newPosition = encoderArray[i].read();

    if (newPosition - oldPositions[i] >= 4) {
      oldPositions[i] = newPosition;  // update the encoder's position
      oneClickLeft(i);
    }

    if (newPosition - oldPositions[i] <= -4) {
      oldPositions[i] = newPosition;
      oneClickRight(i);
    }
  }
}

void oneClickLeft(int encoderIndex) {
  //This is the sequencer encoders which stay 1-6 channel so count is static.
  if (encoderIndex >= 0 && encoderIndex <= 4) {
    usbMIDI.sendNoteOn(midiNotesEncL[encoderIndex], 127, count);
    //This is the FX encoders so they toggle between 1-6 and 7-10
  } else if ((encoderIndex >= 5 && encoderIndex <= 8) && (!fxToADSR)) {
    usbMIDI.sendNoteOn(midiNotesADSRL[encoderIndex - 5], 127, delayCount);
  } else if ((encoderIndex >= 5 && encoderIndex <= 8) && (fxToADSR)) {
    if (!delayCountToggle) {
      usbMIDI.sendNoteOn(midiNotesEncL[encoderIndex], 127, delayCount);
    } else {
      usbMIDI.sendNoteOn(midiNotesEncL[encoderIndex], 127, delayCountLoop);
    }
  } else if (encoderIndex == 9) {
    count = (count % 6) + 1;  // Ensure count stays between 1 and 6
  } else if (encoderIndex == 10) {
    if (!delayCountToggle) {
      delayCount = (delayCount % 6) + 1;  // Ensure count stays between 1 and 6
    } else {
      delayCountLoop = ((delayCountLoop + 2) % 4) + 7;
    }
  } else if (encoderIndex == 11) {
    fxToADSR = !fxToADSR;
  }
}




void oneClickRight(int encoderIndex) {
  //This is the sequencer encoders which stay 1-6 channel so count is static.
  if (encoderIndex >= 0 && encoderIndex <= 4) {
    usbMIDI.sendNoteOn(midiNotesEncR[encoderIndex], 127, count);
    //This is the FX encoders so they toggle between 1-6 and 7-10
  } else if ((encoderIndex >= 5 && encoderIndex <= 8) && (!fxToADSR)) {
    usbMIDI.sendNoteOn(midiNotesADSRR[encoderIndex - 5], 127, delayCount);
  } else if ((encoderIndex >= 5 && encoderIndex <= 8) && (fxToADSR)) {
    if (!delayCountToggle) {
      usbMIDI.sendNoteOn(midiNotesEncR[encoderIndex], 127, delayCount);
    } else {
      usbMIDI.sendNoteOn(midiNotesEncR[encoderIndex], 127, delayCountLoop);
    }
  } else if (encoderIndex == 9) {
    count = (count - 2 + 6) % 6 + 1;  // Ensure count stays between 1 and 6
  } else if (encoderIndex == 10) {
    if (!delayCountToggle) {
      delayCount = (delayCount - 2 + 6) % 6 + 1;  // Ensure count stays between 1 and 6
    } else {
      delayCountLoop = (delayCountLoop % 4) + 7;
    }
  } else if (encoderIndex == 11) {
    fxToADSR = !fxToADSR;
  }
}

//valuesT1 consolidates the remaining pots that don't fit on the teensy 2 pins to one function
void valuesT1() {
  lastCCValueTempo = ccValueTempo;
  potValueTempo = analogRead(potTempo);
  ccValueTempo = map(potValueTempo, 0, 1023, 0, 127);
  if (ccValueTempo != lastCCValueTempo) {
    usbMIDI.sendControlChange(4, ccValueTempo, 15);
  }
  static int lastCCValueT1[2] = { 0, 0 };
  static int accumulatedDifference[2] = { 0, 0 };
  const int threshold = 3;
  const int deadZone = 2;

  for (int i = 0; i < 2; i++) {
    int potValue = analogRead(potT1[i]);
    int ccValue = map(potValue, 0, 1023, 0, 127);

    // Apply dead zone
    if (ccValue <= deadZone) {
      ccValue = 0;
    }

    int valueDifference = abs(ccValue - lastCCValueT1[i]);

    accumulatedDifference[i] += valueDifference;

    if (accumulatedDifference[i] >= threshold && abs(valueDifference) > 1) {
      accumulatedDifference[i] = 0;
      lastCCValueT1[i] = ccValue;

      switch (i) {
        case 0:
          usbMIDI.sendControlChange(12, ccValue, 7);
          break;
        case 1:
          usbMIDI.sendControlChange(13, ccValue, 7);
          break;
      }
    }
  }
}


void allPotLoops() {
  if (Serial1.available() >= 3) {
    int serVal1 = Serial1.read();
    if (serVal1 == 200) {
      int potNumberLoop1 = Serial1.read();
      int potValueLoop1 = Serial1.read();
      switch (potNumberLoop1) {
        case 0:
          usbMIDI.sendControlChange(14, (potValueLoop1), 7);
          break;
        case 1:
          usbMIDI.sendControlChange(15, (potValueLoop1), 7);
          break;
      }
    } else if (serVal1 == 201) {
      int potNumberLoop2 = Serial1.read();
      int potValueLoop2 = Serial1.read();
      switch (potNumberLoop2) {
        case 0:
          usbMIDI.sendControlChange(12, (potValueLoop2), 8);
          break;
        case 1:
          usbMIDI.sendControlChange(13, (potValueLoop2), 8);
          break;
        case 2:
          usbMIDI.sendControlChange(14, (potValueLoop2), 8);
          break;
        case 3:
          usbMIDI.sendControlChange(15, (potValueLoop2), 8);
          break;
      }
    } else if (serVal1 == 202) {
      int potNumberLoop3 = Serial1.read();
      int potValueLoop3 = Serial1.read();
      switch (potNumberLoop3) {
        case 0:
          usbMIDI.sendControlChange(12, (potValueLoop3), 9);
          break;
        case 1:
          usbMIDI.sendControlChange(13, (potValueLoop3), 9);
          break;
        case 2:
          usbMIDI.sendControlChange(14, (potValueLoop3), 9);
          break;
        case 3:
          usbMIDI.sendControlChange(15, (potValueLoop3), 9);
          break;
      }
    } else if (serVal1 == 203) {
      int potNumberLoop4 = Serial1.read();
      int potValueLoop4 = Serial1.read();
      switch (potNumberLoop4) {
        case 0:
          usbMIDI.sendControlChange(12, (potValueLoop4), 10);
          break;
        case 1:
          usbMIDI.sendControlChange(13, (potValueLoop4), 10);
          break;
        case 2:
          usbMIDI.sendControlChange(14, (potValueLoop4), 10);
          break;
        case 3:
          usbMIDI.sendControlChange(15, (potValueLoop4), 10);
          break;
      }
    } else if (serVal1 == 204) {
      int potNumberMix = Serial1.read();
      int potValueMix = Serial1.read();
      switch (potNumberMix) {
        case 0:
          usbMIDI.sendControlChange(0, (potValueMix), 15);
          break;
        case 1:
          usbMIDI.sendControlChange(1, (potValueMix), 15);
          break;
        case 2:
          usbMIDI.sendControlChange(2, (potValueMix), 15);
          break;
        case 3:
          usbMIDI.sendControlChange(3, (potValueMix), 15);
          break;
      }
    }
  }
}

void channelOLED1(void) {
  display1.clearDisplay();

  display1.setTextSize(9);               // Draw 2X-scale text
  display1.setCursor(40, 0);             // Start at top-left corner
  display1.setTextColor(SSD1306_WHITE);  // Set color
  display1.println(count);               // Set printed message

  display1.display();
}

void channelOLED2(void) {
  static bool prevFxToADSR = false;  // Variable to track the previous state of fxToADSR

  display2.clearDisplay();
  display2.setTextSize(9);
  display2.setCursor(40, 0);
  display2.setTextColor(SSD1306_WHITE);

  // Check if fxToADSR has changed
  if (fxToADSR != prevFxToADSR) {
    if (!fxToADSR) {
      display2.setTextSize(5);
      display2.setCursor(5, 10);
      display2.println("ADSR");
    } else {
      display2.setTextSize(4);
      display2.setCursor(5, 15);
      display2.println("DELAY");
    }
    display2.display();
    delay(1000);
    display2.invertDisplay(true);
    fillcircle();
    display2.invertDisplay(false);
  }

  // Update the previous state
  prevFxToADSR = fxToADSR;

  // Clear the display before printing other messages
  display2.clearDisplay();

  if (!delayCountToggle) {
    display2.println(delayCount);  // Set printed message FX channel sequencer
  } else if ((delayCountLoop > 6) && (delayCountLoop < 10)) {
    display2.println(delayCountLoop);  // Set printed message FX channel looper
  } else if (delayCountLoop == 10) {
    display2.setCursor(10, 0);         // Start at the top-left corner
    display2.println(delayCountLoop);  // Set printed message FX channel looper
  }

  display2.display();
}

void fillcircle(void) {
  display1.clearDisplay();

  for (int16_t i = max(display2.width(), display2.height()) / 2; i > 0; i -= 3) {
    // The INVERSE color is used so circles alternate white/black
    display2.fillCircle(display2.width() / 2, display2.height() / 2, i, SSD1306_INVERSE);
    display2.display();  // Update screen with each newly-drawn circle
    delay(1);
  }
}

void clearAllLEDs() {
  memset(ledStates, LOW, sizeof(ledStates));
  memset(ledStatesLiveGrid, LOW, sizeof(ledStatesLiveGrid));
  for (int i = 0; i < Y_DIM * X_DIM; i++) {
    trellis.setPixelColor(i, seesaw_NeoPixel::Color(0, 0, 0));  // off
  }
  trellis.show();
}

void chanToggle() {
  bool toggleState = digitalRead(chTogglePin);
  if (toggleState != previousToggleState) {
    // The state has changed
    if (toggleState == HIGH) {
      updateNeoTrellisLEDs();
    } else {
      updateNeoTrellisLEDsLiveGrid();
    }
    // Update the previous state
    previousToggleState = toggleState;
  }
}



void generateAndProcessMainGrid() {
  for (int iteration = 0; iteration < 4; iteration++) {
    gridChannel = 26 - iteration;
    int listSize = random(7);

    int myList[listSize];

    for (int i = 0; i < listSize; i++) {
      int randomNumber;
      do {
        randomNumber = random(16);
      } while (usedNumbers.count(randomNumber) > 0);
      usedNumbers.insert(randomNumber);

      myList[i] = randomNumber;
    }

    usedNumbers.clear();

    for (int i = 0; i < listSize; i++) {
      int ledIndex = myList[i] + iteration * 20;

      if (ledIndex >= 0 && ledIndex < Y_DIM * X_DIM) {
        ledStates[ledIndex] = !ledStates[ledIndex];

        if (ledStates[ledIndex] == HIGH) {
          int randomNoteMainGrid = gridMIDINotes[myList[i]];
          usbMIDI.sendNoteOn(randomNoteMainGrid, 127, gridChannel);
          delay(5);
        }
      }
    }

    gridChannel = 26;
    delay(25);
  }
}

void generateAndProcessLiveGrid() {
  for (int iteration = 0; iteration < 2; iteration++) {
    gridChannel = 22 - iteration;
    int listSize = random(7);

    int myList[listSize];

    for (int i = 0; i < listSize; i++) {
      int randomNumber;
      do {
        randomNumber = random(16);
      } while (usedNumbers.count(randomNumber) > 0);
      usedNumbers.insert(randomNumber);

      myList[i] = randomNumber;
    }

    usedNumbers.clear();

    for (int i = 0; i < listSize; i++) {
      int ledIndex = myList[i] + iteration * 20;

      if (ledIndex >= 0 && ledIndex < Y_DIM * X_DIM) {
        ledStatesLiveGrid[ledIndex] = !ledStatesLiveGrid[ledIndex];

        if (ledStatesLiveGrid[ledIndex] == HIGH) {
          int randomNoteLiveGrid = gridMIDINotes[myList[i]];
          Serial.print("Live Grid Note: ");
          Serial.println(randomNoteLiveGrid);
          usbMIDI.sendNoteOn(randomNoteLiveGrid, 127, gridChannel);
          delay(5);
        }
      }
    }

    gridChannel = 26;
    delay(25);
  }
}





void updateNeoTrellisLEDs() {
  for (int i = 0; i < 80; i++) {
    trellis.setPixelColor(i, seesaw_NeoPixel::Color(0, 0, 0));
  }
  for (int i = 0; i < Y_DIM * X_DIM; i++) {
    if (ledStates[i] == HIGH) {
      trellis.setPixelColor(i, seesaw_NeoPixel::Color(20, 20, 20));  // on rising
    } else {
      trellis.setPixelColor(i, seesaw_NeoPixel::Color(0, 0, 0));  // off
    }
  }
  trellis.show();
}

void updateNeoTrellisLEDsLiveGrid() {
  // Clear all LEDs
  for (int i = 0; i < 80; i++) {
    trellis.setPixelColor(i, seesaw_NeoPixel::Color(0, 0, 0));
  }

  // Update LEDs based on the ledStatesLiveGrid array
  for (int i = 0; i < Y_DIM_LIVEGRID * X_DIM_LIVEGRID; i++) {
    if (ledStatesLiveGrid[i] == HIGH) {
      trellis.setPixelColor(i, seesaw_NeoPixel::Color(20, 20, 20));  // on rising
    } else {
      trellis.setPixelColor(i, seesaw_NeoPixel::Color(0, 0, 0));  // off
    }
  }

  // Show the updated LEDs
  trellis.show();
}
