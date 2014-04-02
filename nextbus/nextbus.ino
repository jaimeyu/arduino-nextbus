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




#ifdef EPD_ORIG_MODE
// Arduino IO layout
const int PIN_PANEL_TEMPERATURE = A0;
const int PIN_PANEL_ON = 2;
const int PIN_PANEL_BORDER = 3;
const int PIN_PANEL_DISCHARGE = 4;
const int PIN_PANEL_PWM = 5;
const int PIN_PANEL_RESET = 6;
const int PIN_PANEL_BUSY = 7;
const int PIN_EPD_CS = 8;
const int PIN_FLASH_CS = 9;
const int PIN_PANEL_SW2 = 12; // <-- TODO: What is this?? Check schem.
const int PIN_RED_LED = 13;

#else
typedef enum {
  SER_RX = 0,
  SER_TX = 1,
  PIN_PANEL_ON = 2,
  PIN_PANEL_BORDER = 3,
  PIN_PANEL_DISCHARGE = 4,
  PIN_PANEL_PWM = 5,
  PIN_PANEL_RESET = 6,
  PIN_PANEL_BUSY = 7,
  PIN_EPD_CS = 8,
  PIN_FLASH_CS = 9,
  __GPIO_AVAIL_10 = 10,
  __GPIO_AVAIL_11 = 11,
  PIN_PANEL_SW2 = 12, // <-- TODO: What is this?? Check schem.
  PIN_RED_LED = 13,
  
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  __GPIO_AVAIL_14 = 14,
  __GPIO_AVAIL_15 = 15,
  // TODO: There are a total of 54 GPIOs on the MEGA. 
#endif
} DIGITAL_PIN_OUT;

typedef enum {
  PIN_PANEL_TEMPERATURE = A0,
  __ANALOG_AVAIL_A1 = A1,
  __ANALOG_AVAIL_A2 = A2,
  __ANALOG_AVAIL_A3 = A3,
  __ANALOG_AVAIL_A4 = A4,
  __ANALOG_AVAIL_A5 = A5,

#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  __ANALOG_AVAIL_A6 = A6,
  __ANALOG_AVAIL_A7 = A7,
  __ANALOG_AVAIL_A8 = A8,
  __ANALOG_AVAIL_A9 = A9,
  __ANALOG_AVAIL_A10 = A10,
  __ANALOG_AVAIL_A11 = A11,
  __ANALOG_AVAIL_A12 = A12,
  __ANALOG_AVAIL_A13 = A13,
#endif
 
} ANALOG_PIN_OUT;

#endif




// LED anode through resistor to I/O pin
// LED cathode to Ground
#define LED_ON  HIGH
#define LED_OFF LOW


// pre-processor convert to string
#define MAKE_STRING1(X) #X
#define MAKE_STRING(X) MAKE_STRING1(X)


// define the E-Ink display
EPD_Class EPD(EPD_SIZE, PIN_PANEL_ON, PIN_PANEL_BORDER, PIN_PANEL_DISCHARGE, PIN_PANEL_PWM, PIN_PANEL_RESET, PIN_PANEL_BUSY, PIN_EPD_CS);

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
  pinMode(PIN_PANEL_SW2, INPUT);
  pinMode(PIN_PANEL_TEMPERATURE, INPUT);
  pinMode(PIN_PANEL_PWM, OUTPUT);
  pinMode(PIN_PANEL_BUSY, INPUT);
  pinMode(PIN_PANEL_RESET, OUTPUT);
  pinMode(PIN_PANEL_ON, OUTPUT);
  pinMode(PIN_PANEL_DISCHARGE, OUTPUT);
  pinMode(PIN_PANEL_BORDER, OUTPUT);
  pinMode(PIN_EPD_CS, OUTPUT);
  pinMode(PIN_FLASH_CS, OUTPUT);

  digitalWrite(PIN_RED_LED, LOW);
  digitalWrite(PIN_PANEL_PWM, LOW);
  digitalWrite(PIN_PANEL_RESET, LOW);
  digitalWrite(PIN_PANEL_ON, LOW);
  digitalWrite(PIN_PANEL_DISCHARGE, LOW);
  digitalWrite(PIN_PANEL_BORDER, LOW);
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
  S5813A.begin(PIN_PANEL_TEMPERATURE);

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
