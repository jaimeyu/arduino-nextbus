// Copyright 2013 Pervasive Displays, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at:
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
// express or implied.  See the License for the specific language
// governing permissions and limitations under the License.

/** TODO: Why inttypes AND ctypes? Reduce to gcc-int? **/
#include <inttypes.h>
#include <ctype.h>



/** Wifi module **/
#include <Adafruit_CC3000.h>
#include <Adafruit_CC3000_Server.h>
#include <ccspi.h>


/** Display **/

// required libraried
#include <SPI.h>
#include <FLASH.h>
#include <EPD.h>
#include <S5813A.h>
#include <Adafruit_GFX.h>
#include <EPD_GFX.h>


// Change this for different display size
// supported sizes: 1_44 2_0
// NOTE: change the pixel_width in EPD_GFX.h to match
//       selected display (1.44 -> 128, 2.00 -> 200)
#define EPD_SIZE EPD_2_0


// update delay in seconds
#define LOOP_DELAY_SECONDS 60


// no futher changes below this point

// current version number
#define THERMO_VERSION "3"


// Arduino IO layout
const int PIN_DISPLAY_TEMPERATURE = A0;
const int PIN_PANEL_ON = 2;
const int PIN_DISPLAY_BORDER = 3;
const int PIN_DISPLAY_DISCHARGE = 4;
const int PIN_DISPLAY_PWM = 5;
const int PIN_DISPLAY_RESET = 6;
const int PIN_DISPLAY_BUSY = 7;
const int PIN_EPD_CS = 8;
const int PIN_FLASH_CS = 9;
const int PIN_DISPLAY_SW2 = 12; // <-- TODO: What is this?? Check schem.
const int PIN_RED_LED = 13;

// LED anode through resistor to I/O pin
// LED cathode to Ground
#define LED_ON  HIGH
#define LED_OFF LOW


// pre-processor convert to string
#define MAKE_STRING1(X) #X
#define MAKE_STRING(X) MAKE_STRING1(X)


// define the E-Ink display
EPD_Class EPD(EPD_SIZE, PIN_PANEL_ON, PIN_DISPLAY_BORDER, PIN_DISPLAY_DISCHARGE, PIN_DISPLAY_PWM, PIN_DISPLAY_RESET, PIN_DISPLAY_BUSY, PIN_EPD_CS);

// graphic handler
EPD_GFX G_EPD(EPD, S5813A);

// Prototype for now.
void displayHandler(void);
void displaySetup(void);
void displayFlashSetup(void);
void displayTempSensorSetup(void);

// I/O setup
void setup() {
  Serial.begin(9600);

  displaySetup();
  displayFlashSetup();
  displayTempSensorSetup();
}


// main loop
void loop() {
  void displayHandler();
}

void displaySetup() {
  pinMode(PIN_RED_LED, OUTPUT);
  pinMode(PIN_DISPLAY_SW2, INPUT);
  pinMode(PIN_DISPLAY_TEMPERATURE, INPUT);
  pinMode(PIN_DISPLAY_PWM, OUTPUT);
  pinMode(PIN_DISPLAY_BUSY, INPUT);
  pinMode(PIN_DISPLAY_RESET, OUTPUT);
  pinMode(PIN_PANEL_ON, OUTPUT);
  pinMode(PIN_DISPLAY_DISCHARGE, OUTPUT);
  pinMode(PIN_DISPLAY_BORDER, OUTPUT);
  pinMode(PIN_EPD_CS, OUTPUT);
  pinMode(PIN_FLASH_CS, OUTPUT);

  digitalWrite(PIN_RED_LED, LOW);
  digitalWrite(PIN_DISPLAY_PWM, LOW);
  digitalWrite(PIN_DISPLAY_RESET, LOW);
  digitalWrite(PIN_PANEL_ON, LOW);
  digitalWrite(PIN_DISPLAY_DISCHARGE, LOW);
  digitalWrite(PIN_DISPLAY_BORDER, LOW);
  digitalWrite(PIN_EPD_CS, LOW);
  digitalWrite(PIN_FLASH_CS, HIGH);


  Serial.println();
  Serial.println();
  Serial.println("Display: " MAKE_STRING(EPD_SIZE));
  Serial.println();




  // set up graphics EPD library
  // and clear the screen
  G_EPD.begin();
}


void displayFlashSetup(void) {
  FLASH.begin(PIN_FLASH_CS);
  if (FLASH.available()) {
    Serial.println("FLASH chip detected OK");
  } else {
    Serial.println("unsupported FLASH chip");
  }
}

void displayTempSensorSetup(void) {
  Serial.println("Thermo version: " THERMO_VERSION);

  // configure temperature sensor
  S5813A.begin(PIN_DISPLAY_TEMPERATURE);

  // get the current temperature
  int temperature = S5813A.read();
  Serial.print("Temperature = ");
  Serial.print(temperature);
  Serial.println(" Celcius");
}

void displayHandler() {
  int temperature = S5813A.read();
  Serial.print("Temperature = ");
  Serial.print(temperature);
  Serial.println(" Celcius");

  int h = G_EPD.height();
  int w = G_EPD.width();

  G_EPD.drawRect(1, 1, w - 2, h - 2, EPD_GFX::BLACK);
  G_EPD.drawRect(3, 3, w - 6, h - 6, EPD_GFX::BLACK);

  G_EPD.fillTriangle(135, 20, 186, 40, 152, 84, EPD_GFX::BLACK);
  G_EPD.fillTriangle(139, 26, 180, 44, 155, 68, EPD_GFX::WHITE);

  char temp[sizeof("-999 C")];
  snprintf(temp, sizeof(temp), "%4d C", temperature);

  int x = 20;
  int y = 30;
  for (int i = 0; i < sizeof(temp) - 1; ++i, x += 14) {
    G_EPD.drawChar(x, y, temp[i], EPD_GFX::BLACK, EPD_GFX::WHITE, 2);
  }

  // small circle for degrees symbol
  G_EPD.drawCircle(20 + 4 * 14 + 6, 30, 4, EPD_GFX::BLACK);

  // 100 difference just to simplify things
  // so 1 pixel = 1 degree
#define T_MIN (-10)
#define T_MAX 80

  // clip
  if (temperature < T_MIN) {
    temperature = T_MIN;
  } else if (temperature > T_MAX) {
    temperature = T_MAX;
  }

  // temperature bar
  int bar_w = temperature - T_MIN;  // zero based
  int bar_h = 4;
  int bar_x0 = 24;
  int bar_y0 = 60;

  G_EPD.fillRect(bar_x0, bar_y0, T_MAX - T_MIN, bar_h, EPD_GFX::WHITE);
  G_EPD.fillRect(bar_x0, bar_y0, bar_w, bar_h, EPD_GFX::BLACK);

  // scale
  for (int t0 = T_MIN; t0 < T_MAX; t0 += 5) {
    int t = t0 - T_MIN;
    int tick = 8;
    if (0 == t0) {
      tick = 12;
      G_EPD.drawCircle(bar_x0 + t, bar_y0 + 16, 3, EPD_GFX::BLACK);
    } else if (0 == t0 % 10) {
      tick = 10;
    }
    G_EPD.drawLine(bar_x0 + t, bar_y0 + tick, bar_x0 + t, bar_y0 + 6, EPD_GFX::BLACK);
    G_EPD.drawLine(bar_x0 + t, bar_y0 + 6, bar_x0 + t + 5, bar_y0 + 6, EPD_GFX::BLACK);
    G_EPD.drawLine(bar_x0 + t + 5, bar_y0 + 6, bar_x0 + t + 5, bar_y0 + 8, EPD_GFX::BLACK);
  }

  // update the display
  G_EPD.display();

  // flash LED for a number of seconds
  for (int x = 0; x < LOOP_DELAY_SECONDS * 10; ++x) {
    digitalWrite(PIN_RED_LED, LED_ON);
    delay(50);
    digitalWrite(PIN_RED_LED, LED_OFF);
    delay(50);
  }
}
