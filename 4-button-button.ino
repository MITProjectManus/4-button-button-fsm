// othomas@mit.edu // 2025-02-19

// Purpose: This code is a simple example of how to use the ESP32-S2 to
// call an HTTPS POST endpoint in Airtable. The code is designed to be
// run on an ESP32-S2 Feather board with a built-in NeoPixel and four
// buttons. The code is designed to be a starting point for more complex
// projects for hardware interacting with Airtable.

// Circuit: ESP32-S2 Feather with four buttons connected to it. The
// buttons are referenced by color in the code, because Oliver happened
// to have colored buttons. Pin mapping is as follows:
// Button A / RED / CLOSE SHOP:           10
// Button B / YELLOW / SOFT OPEN SHOP:     9
// Button C / GREEN / OPEN SHOP:           6
// Button D / BLUE / CLEAR CHECKIN SCREEN: 5


// Airtable:
// =========
// Endpoint: Set Makerspace Status
// URL: 
// Method: POST
// Payload: {"msSlug" : "Slug", "status": "Status"}
//
// Reminder: the key to the left of the colon is passed literally, the value to
// the right of the colon should be replaced with a valid slug or status value.
//
// Endpoint: Clear Checked-in Makers List
// URL:
// Method: POST
// Payload: {"msSlug" : "Slug", "reason": "Reason Code"}
//
// Makerspace Slugs:
// Metropolis = "metropolis"
// The Deep   = "thedeep"
// Die Zauberstube (test makerspace) = "zauberstube"
//
// Valid Statuses:
// "Open" or "Closed" or "Soft Open" or "Reserved"
// 
// Valid Reason Codes:
// "418"

// Notes:
// - Move most of this exaplantion to a GitHub README.md file
// - Add a license to the code
// - Add a README.md file
// - Add a .gitignore file
// - Add an 4-button-button.h file with the sensitive Airtable and WiFi info and add to .gitignore 

#include <WiFi.h>
#include <HTTPClient.h>
#include <Adafruit_NeoPixel.h>
#include "4-button-button.h"

Adafruit_NeoPixel pixels(1, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

#define BUTTON_A  10 // RED / CLOSE
#define BUTTON_B   9 // YELLOW / SOFT OPEN
#define BUTTON_C   6 // GREEN / OPEN
#define BUTTON_D   5 // BLUE / CLEAR

const char WIFI_SSID[]     = "MIT";
const char WIFI_PASSWORD[] = wifiPassword;

String CLOSE_MAKERSPACE = "{\"msSlug\":\"zauberstube\",\"status\":\"Closed\"}";
String SOFTO_MAKERSPACE = "{\"msSlug\":\"zauberstube\",\"status\":\"Soft Open\"}";
String OPEN_MAKERSPACE  = "{\"msSlug\":\"zauberstube\",\"status\":\"Open\"}";
String CLEAR_MAKERSPACE = "{\"msSlug\":\"zauberstube\",\"status\":\"418\"}";

// State machine states
enum States {
  BOOT,     // Booting...
  READY,    // Ready for Input
  CLOSE,    // Status / Close Wait
  SOFTOPEN, // Status / Soft Open Wait
  OPEN,     // Status / Open Wait
  CLEAR,    // Checkin / Clear Wait
  ERROR     // Button or Network Error
};

// Set initial starting state
State state = States::BOOT;

void setup() {
  Serial.begin(115200);
  delay(5000); // Give the serial port a little time to settle.

  Serial.println("");
  Serial.println("Serial console initialized.");
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi network ");
  Serial.print(WIFI_SSID);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("");

  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  pixels.begin(); // Init onboard NeoPixel

  // Set setup exit state
  State state = States::READY;
}

HTTPClient http;
int httpCode;
unsigned long previousMillis = 0;
const long blockDelay = 300000; // Mandatory pause of 5m between API calls to allow things to settle
const long flashInterval = 500; // Flash the LED once per second if it is flashing

void loop() {


  // Some simple test code to run through the pixel states
  pixels.clear(); // Pixel off
  delay(interval);
  pixels.setPixelColor(0, pixels.Color(192, 0, 0));
  pixels.show();
  delay(interval*4);

  pixels.clear();
  delay(interval);
  pixels.setPixelColor(0, pixels.Color(192, 128, 0));
  pixels.show();
  delay(interval*4);

  pixels.clear();
  delay(interval);
  pixels.setPixelColor(0, pixels.Color(0, 128, 0));
  pixels.show();
  delay(interval*4);

  pixels.clear();
  delay(interval);
  pixels.setPixelColor(0, pixels.Color(0, 0, 255));
  pixels.show();
  delay(interval*4);

  unsigned long currentMillis = millis();
  if(currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    http.begin(urlShopStatus);
    http.addHeader("Content-Type", "application/json");

    if(inuse) {
      Serial.println("Calling Airtable web hook with status IN USE.");
      // httpCode = http.POST(TEST_INUSE);
      inuse = false;
    } else {
      Serial.println("Calling Airtable web hook with status NOT IN USE.");
      // httpCode = http.POST(TEST_NOTINUSE);
      inuse = true;
    }

    // httpCode will be negative on error
    // if (httpCode > 0) {
    //   if (httpCode == HTTP_CODE_OK) {
    //     String payload = http.getString();
    //     Serial.println(payload);
    //   } else {
    //     Serial.printf("[HTTP] POST... code: %d\n", httpCode);
    //   }
    // } else {
    //   Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
    // }
    // http.end();
  }
}
