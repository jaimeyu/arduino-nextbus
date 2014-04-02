// Copyright 2014 Jaime Yu
// jaime@jaimeyu.com
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


/**********************************/
/* DISPLAY DEFINES */
/**********************************/

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

// LED anode through resistor to I/O pin
// LED cathode to Ground
#define LED_ON  HIGH
#define LED_OFF LOW


// pre-processor convert to string
#define MAKE_STRING1(X) #X
#define MAKE_STRING(X) MAKE_STRING1(X)

/**********************************/
/* GLOBALS */
/**********************************/

// Arduino GPIO layout
/** Display SEGMENT **/
const uint8_t GPIO_PANEL_ON = 2;
const uint8_t GPIO_PANEL_BORDER = 3;
const uint8_t GPIO_PANEL_DISCHARGE = 4;
const uint8_t GPIO_PANEL_PWM = 5;
const uint8_t GPIO_PANEL_RESET = 6;
const uint8_t GPIO_PANEL_BUSY = 7;
const uint8_t GPIO_EPD_CS = 8;
const uint8_t GPIO_FLASH_CS = 9;
const uint8_t __GPIO_AVAIL_10 = 10;
const uint8_t __GPIO_AVAIL_11 = 11;
const uint8_t GPIO_PANEL_SW2 = 12; // <-- TODO: What is this?? Check schem.
const uint8_t GPIO_RED_LED = 13;
/** WIFI SEGMENT **/
const uint8_t GPIO_CC3000_VBAT = 14;
const uint8_t GPIO_CC3000_CS = 15;
const uint8_t __GPIO_AVAIL_16 = 16;
const uint8_t __GPIO_AVAIL_17 = 17;
const uint8_t GPIO_CC3000_IRQ = 18; // INTR 5 on MEGA
const uint8_t __GPIO_AVAIL_19 = 19;
const uint8_t __GPIO_AVAIL_20 = 20;

// Analog layout
const uint8_t AIO_PANEL_TEMPERATURE = A0;
const uint8_t __ANALOG_AVAIL_A1 = A1;
const uint8_t __ANALOG_AVAIL_A2 = A2;
const uint8_t __ANALOG_AVAIL_A3 = A3;
const uint8_t __ANALOG_AVAIL_A4 = A4;
const uint8_t __ANALOG_AVAIL_A5 = A5;

#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
const uint8_t __ANALOG_AVAIL_A6 = A6;
const uint8_t __ANALOG_AVAIL_A7 = A7;
const uint8_t __ANALOG_AVAIL_A8 = A8;
const uint8_t __ANALOG_AVAIL_A9 = A9;
const uint8_t __ANALOG_AVAIL_A10 = A10;
const uint8_t __ANALOG_AVAIL_A11 = A11;
const uint8_t __ANALOG_AVAIL_A12 = A12;
const uint8_t __ANALOG_AVAIL_A13 = A13;
#endif

/** DISPLAY GLOBALS **/
EPD_Class EPD(EPD_SIZE, GPIO_PANEL_ON, GPIO_PANEL_BORDER, GPIO_PANEL_DISCHARGE, GPIO_PANEL_PWM, GPIO_PANEL_RESET, GPIO_PANEL_BUSY, GPIO_EPD_CS);

// graphic handler
EPD_GFX G_EPD(EPD, S5813A);


/** WIFI GLOBALS **/
Adafruit_CC3000 cc3000 = Adafruit_CC3000(GPIO_CC3000_CS, GPIO_CC3000_IRQ, GPIO_CC3000_VBAT,
                                         SPI_CLOCK_DIVIDER); // you can change this clock speed but DI

#define WLAN_SSID       "myNetwork"        // cannot be longer than 32 characters!
#define WLAN_PASS       "myPassword"
// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_WPA2

Adafruit_CC3000_Client client;

const unsigned long
  connectTimeout  = 15L * 1000L, // Max time to wait for server connection
  responseTimeout = 15L * 1000L; // Max time to wait for data from server
int
  countdown       = 0;  // loop() iterations until next time server query
unsigned long
  lastPolledTime  = 0L, // Last value retrieved from time server
  sketchTime      = 0L; // CPU milliseconds since last server query


// Prototype for now.
void displayHandler(void);
void displaySetup(void);
void displayFlashSetup(void);
void displayTempSensorSetup(void);

void wifiSetup(void);
void wifiHandler(void);

unsigned long getTime(void);
bool displayConnectionDetails(void);
void displayMACAddress(void);
uint16_t checkFirmwareVersion(void);

// I/O setup
void setup() {
  Serial.begin(9600);

  displaySetup();
  displayFlashSetup();
  displayTempSensorSetup();
  
  wifiSetup();
}


// main loop
void loop() {
  void displayHandler();
  void wifiHandler();
}

void displaySetup() {
  pinMode(GPIO_RED_LED, OUTPUT);
  pinMode(GPIO_PANEL_SW2, INPUT);
  pinMode(AIO_PANEL_TEMPERATURE, INPUT);
  pinMode(GPIO_PANEL_PWM, OUTPUT);
  pinMode(GPIO_PANEL_BUSY, INPUT);
  pinMode(GPIO_PANEL_RESET, OUTPUT);
  pinMode(GPIO_PANEL_ON, OUTPUT);
  pinMode(GPIO_PANEL_DISCHARGE, OUTPUT);
  pinMode(GPIO_PANEL_BORDER, OUTPUT);
  pinMode(GPIO_EPD_CS, OUTPUT);
  pinMode(GPIO_FLASH_CS, OUTPUT);

  digitalWrite(GPIO_RED_LED, LOW);
  digitalWrite(GPIO_PANEL_PWM, LOW);
  digitalWrite(GPIO_PANEL_RESET, LOW);
  digitalWrite(GPIO_PANEL_ON, LOW);
  digitalWrite(GPIO_PANEL_DISCHARGE, LOW);
  digitalWrite(GPIO_PANEL_BORDER, LOW);
  digitalWrite(GPIO_EPD_CS, LOW);
  digitalWrite(GPIO_FLASH_CS, HIGH);


  Serial.println();
  Serial.println();
  Serial.println("Display: " MAKE_STRING(EPD_SIZE));
  Serial.println();




  // set up graphics EPD library
  // and clear the screen
  G_EPD.begin();
}


void displayFlashSetup(void) {
  FLASH.begin(GPIO_FLASH_CS);
  if (FLASH.available()) {
    Serial.println("FLASH chip detected OK");
  } else {
    Serial.println("unsupported FLASH chip");
  }
}

void displayTempSensorSetup(void) {
  Serial.println("Thermo version: " THERMO_VERSION);

  // configure temperature sensor
  S5813A.begin(AIO_PANEL_TEMPERATURE);

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
    digitalWrite(GPIO_RED_LED, LED_ON);
    delay(50);
    digitalWrite(GPIO_RED_LED, LED_OFF);
    delay(50);
  }
}






void wifiSetup(void) {
  
  Serial.println(F("\nInitialising the CC3000 ..."));
  if (!cc3000.begin()) {
    Serial.println(F("Unable to initialise the CC3000! Check your wiring?"));
    for(;;);
  }

  uint16_t firmware = checkFirmwareVersion();
  if ((firmware != 0x113) && (firmware != 0x118)) {
    Serial.println(F("Wrong firmware version!"));
    for(;;);
  }
  
  displayMACAddress();
  
  Serial.println(F("\nDeleting old connection profiles"));
  if (!cc3000.deleteProfiles()) {
    Serial.println(F("Failed!"));
    while(1);
  }

  /* Attempt to connect to an access point */
  char *ssid = WLAN_SSID;             /* Max 32 chars */
  Serial.print(F("\nAttempting to connect to ")); Serial.println(ssid);
  
  /* NOTE: Secure connections are not available in 'Tiny' mode! */
  if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
    Serial.println(F("Failed!"));
    while(1);
  }
   
  Serial.println(F("Connected!"));
  
  /* Wait for DHCP to complete */
  Serial.println(F("Request DHCP"));
  while (!cc3000.checkDHCP()) {
    delay(100); // ToDo: Insert a DHCP timeout!
  }

  /* Display the IP address DNS, Gateway, etc. */  
  while (!displayConnectionDetails()) {
    delay(1000);
  }
}

void wifiHandler(void) {
   if(countdown == 0) {            // Time's up?
    unsigned long t  = getTime(); // Query time server
    if(t) {                       // Success?
      lastPolledTime = t;         // Save time
      sketchTime     = millis();  // Save sketch time of last valid time query
      countdown      = 24*60*4-1; // Reset counter: 24 hours * 15-second intervals
    }
  } else {
    countdown--;                  // Don't poll; use math to figure current time
  }

  unsigned long currentTime = lastPolledTime + (millis() - sketchTime) / 1000;

  Serial.print(F("Current UNIX time: "));
  Serial.print(currentTime);
  Serial.println(F(" (seconds since 1/1/1970 UTC)"));

  delay(15000L); // Pause 15 seconds
}




/**************************************************************************/
/*!
    @brief  Tries to read the CC3000's internal firmware patch ID
*/
/**************************************************************************/
uint16_t checkFirmwareVersion(void)
{
  uint8_t major, minor;
  uint16_t version;
  
#ifndef CC3000_TINY_DRIVER  
  if(!cc3000.getFirmwareVersion(&major, &minor))
  {
    Serial.println(F("Unable to retrieve the firmware version!\r\n"));
    version = 0;
  }
  else
  {
    Serial.print(F("Firmware V. : "));
    Serial.print(major); Serial.print(F(".")); Serial.println(minor);
    version = major; version <<= 8; version |= minor;
  }
#endif
  return version;
}

/**************************************************************************/
/*!
    @brief  Tries to read the 6-byte MAC address of the CC3000 module
*/
/**************************************************************************/
void displayMACAddress(void)
{
  uint8_t macAddress[6];
  
  if(!cc3000.getMacAddress(macAddress))
  {
    Serial.println(F("Unable to retrieve MAC Address!\r\n"));
  }
  else
  {
    Serial.print(F("MAC Address : "));
    cc3000.printHex((byte*)&macAddress, 6);
  }
}


/**************************************************************************/
/*!
    @brief  Tries to read the IP address and other connection details
*/
/**************************************************************************/
bool displayConnectionDetails(void)
{
  uint32_t ipAddress, netmask, gateway, dhcpserv, dnsserv;
  
  if(!cc3000.getIPAddress(&ipAddress, &netmask, &gateway, &dhcpserv, &dnsserv))
  {
    Serial.println(F("Unable to retrieve the IP Address!\r\n"));
    return false;
  }
  else
  {
    Serial.print(F("\nIP Addr: ")); cc3000.printIPdotsRev(ipAddress);
    Serial.print(F("\nNetmask: ")); cc3000.printIPdotsRev(netmask);
    Serial.print(F("\nGateway: ")); cc3000.printIPdotsRev(gateway);
    Serial.print(F("\nDHCPsrv: ")); cc3000.printIPdotsRev(dhcpserv);
    Serial.print(F("\nDNSserv: ")); cc3000.printIPdotsRev(dnsserv);
    Serial.println();
    return true;
  }
}

// Minimalist time server query; adapted from Adafruit Gutenbird sketch,
// which in turn has roots in Arduino UdpNTPClient tutorial.
unsigned long getTime(void) {

  uint8_t       buf[48];
  unsigned long ip, startTime, t = 0L;

  Serial.print(F("Locating time server..."));

  // Hostname to IP lookup; use NTP pool (rotates through servers)
  if(cc3000.getHostByName("pool.ntp.org", &ip)) {
    static const char PROGMEM
      timeReqA[] = { 227,  0,  6, 236 },
      timeReqB[] = {  49, 78, 49,  52 };

    Serial.println(F("\r\nAttempting connection..."));
    startTime = millis();
    do {
      client = cc3000.connectUDP(ip, 123);
    } while((!client.connected()) &&
            ((millis() - startTime) < connectTimeout));

    if(client.connected()) {
      Serial.print(F("connected!\r\nIssuing request..."));

      // Assemble and issue request packet
      memset(buf, 0, sizeof(buf));
      memcpy_P( buf    , timeReqA, sizeof(timeReqA));
      memcpy_P(&buf[12], timeReqB, sizeof(timeReqB));
      client.write(buf, sizeof(buf));

      Serial.print(F("\r\nAwaiting response..."));
      memset(buf, 0, sizeof(buf));
      startTime = millis();
      while((!client.available()) &&
            ((millis() - startTime) < responseTimeout));
      if(client.available()) {
        client.read(buf, sizeof(buf));
        t = (((unsigned long)buf[40] << 24) |
             ((unsigned long)buf[41] << 16) |
             ((unsigned long)buf[42] <<  8) |
              (unsigned long)buf[43]) - 2208988800UL;
        Serial.print(F("OK\r\n"));
      }
      client.close();
    }
  }
  if(!t) Serial.println(F("error"));
  return t;
}
