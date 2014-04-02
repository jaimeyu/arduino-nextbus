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
const int Pin_TEMPERATURE = A0;
const int Pin_PANEL_ON = 2;
const int Pin_BORDER = 3;
const int Pin_DISCHARGE = 4;
const int Pin_PWM = 5;
const int Pin_RESET = 6;
const int Pin_BUSY = 7;
const int Pin_EPD_CS = 8;
const int Pin_FLASH_CS = 9;
const int Pin_SW2 = 12;
const int Pin_RED_LED = 13;

// LED anode through resistor to I/O pin
// LED cathode to Ground
#define LED_ON  HIGH
#define LED_OFF LOW


// pre-processor convert to string
#define MAKE_STRING1(X) #X
#define MAKE_STRING(X) MAKE_STRING1(X)


// define the E-Ink display
EPD_Class EPD(EPD_SIZE, Pin_PANEL_ON, Pin_BORDER, Pin_DISCHARGE, Pin_PWM, Pin_RESET, Pin_BUSY, Pin_EPD_CS);

// graphic handler
EPD_GFX G_EPD(EPD, S5813A);

// Prototype for now. 
void displayHandler(void);

// I/O setup
void setup() {
	pinMode(Pin_RED_LED, OUTPUT);
	pinMode(Pin_SW2, INPUT);
	pinMode(Pin_TEMPERATURE, INPUT);
	pinMode(Pin_PWM, OUTPUT);
	pinMode(Pin_BUSY, INPUT);
	pinMode(Pin_RESET, OUTPUT);
	pinMode(Pin_PANEL_ON, OUTPUT);
	pinMode(Pin_DISCHARGE, OUTPUT);
	pinMode(Pin_BORDER, OUTPUT);
	pinMode(Pin_EPD_CS, OUTPUT);
	pinMode(Pin_FLASH_CS, OUTPUT);

	digitalWrite(Pin_RED_LED, LOW);
	digitalWrite(Pin_PWM, LOW);
	digitalWrite(Pin_RESET, LOW);
	digitalWrite(Pin_PANEL_ON, LOW);
	digitalWrite(Pin_DISCHARGE, LOW);
	digitalWrite(Pin_BORDER, LOW);
	digitalWrite(Pin_EPD_CS, LOW);
	digitalWrite(Pin_FLASH_CS, HIGH);

	Serial.begin(9600);

	Serial.println();
	Serial.println();
	Serial.println("Thermo version: " THERMO_VERSION);
	Serial.println("Display: " MAKE_STRING(EPD_SIZE));
	Serial.println();

	FLASH.begin(Pin_FLASH_CS);
	if (FLASH.available()) {
		Serial.println("FLASH chip detected OK");
	} else {
		Serial.println("unsupported FLASH chip");
	}

	// configure temperature sensor
	S5813A.begin(Pin_TEMPERATURE);

	// get the current temperature
	int temperature = S5813A.read();
	Serial.print("Temperature = ");
	Serial.print(temperature);
	Serial.println(" Celcius");

	// set up graphics EPD library
	// and clear the screen
	G_EPD.begin();
}


// main loop
void loop() {
	int temperature = S5813A.read();
	Serial.print("Temperature = ");
	Serial.print(temperature);
	Serial.println(" Celcius");

	int h = G_EPD.height();
	int w = G_EPD.width();

	G_EPD.drawRect(1, 1, w - 2, h - 2, EPD_GFX::BLACK);
	G_EPD.drawRect(3, 3, w - 6, h - 6, EPD_GFX::BLACK);

	G_EPD.fillTriangle(135,20, 186,40, 152,84, EPD_GFX::BLACK);
	G_EPD.fillTriangle(139,26, 180,44, 155,68, EPD_GFX::WHITE);

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
		temperature= T_MIN;
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
		  digitalWrite(Pin_RED_LED, LED_ON);
		  delay(50);
		  digitalWrite(Pin_RED_LED, LED_OFF);
		  delay(50);
	}
}
