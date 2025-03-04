// othomas@mit.edu 
// 2025-02-19

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

// Airtable Notes:
// 
// Two webhooks are set up in Airtable, one to update a makerspace's status and one to clear a makerspace's
// checkin screen. At the moment, the web hook URL which includes a hash and requiring a record ID in the 
// payload are the only "security features". Calls are non-destructive so this is not a significant exposure.
// These parameters are in a required "4-button-button.h" file which is NOT tracked in GitHub, but a sample
// without the MIT record IDs and webhooks is shown in the README and saved as "4-button-button-sample.h".
//
// TODO:
// [ ] Move the https calling code in the CLOSE, SOFTOPEN, OPEN, and CLEAR states to a single calling
//     and return status function that is called from all for state cases, passing URL and JSON payload as
//     arguments; on network failure set state to ERROR.
// [ ] Add wifi backoff and retry code to ERROR case; (copy from my Ethernet.h wESP32 code).

#include <WiFi.h>
#include <HTTPClient.h>
#include <Adafruit_NeoPixel.h>
#include "4-button-button.h"

Adafruit_NeoPixel pixels(1, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

// Button GPIO Pins
#define BUTTON_A  10 // RED / CLOSE
#define BUTTON_B   9 // YELLOW / SOFT OPEN
#define BUTTON_C   6 // GREEN / OPEN
#define BUTTON_D   5 // BLUE / CLEAR

// State machine states
enum States {
  BOOT,         // Booting...
  READY,        // Ready for Input
  CLOSE,        // CLOSE button pressed
  CLOSEWAIT,    // Post-Close wait state
  SOFTOPEN,     // SOFT OPEN button pressed
  SOFTOPENWAIT, // Post-Soft Open wait state
  OPEN,         // OPEN button pressed
  OPENWAIT,     // Post-Open wait state
  CLEAR,        // Checkin CLEAR button pressed
  CLEARWAIT,    // Post-Clear wait state
  ERROR         // Network Error
};

// Set initial starting state
States state = States::BOOT;

// We'll be timining several things, so we need to keep track of time for each
// Using timers for everything takes a little more memory, but allows us to avoid
// all blocking code in the loop. (I.e., no delay() calls.)
unsigned long currentMillis = 0;    // Set to current time in ms frequently throughout the code
unsigned long wifiStartMillis = 0;  // Holds the last time a wifi connection attempt was started
unsigned long errorStartMillis = 0; // Holds last time error state started
unsigned long waitStartMillis = 0;  // Holds last time a wait state started
unsigned long lastFlashMillis = 0;  // Holds last time the pixel changed state

const long wifiInterval =  20000;   // We give WiFi about 20 seconds to connect or reconnect
const long wifiBackoff  = 300000;   // If we can't connect, back off for 5 minutes before trying again
const long waitInterval = 300000;   // Long-ish 5m delay enforced between actions; too many API calls cause mayhem
const long flashInterval =   500;   // Flash 1/2 interval

bool pixelOn = false;              // Tracks pixel state for timer-based pixel cycling; we will exit setup() with pixel off

void setup() {
  Serial.begin(115200);
  delay(1000); // Give the serial port a little time to settle.

  Serial.println("");
  Serial.println("Serial console initialized.");
  
  pixels.begin(); // Init onboard NeoPixel
  pixels.clear();

  // Configure button interrupts and handlers
  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);
  pinMode(BUTTON_D, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_A), buttonA, FALLING);
  attachInterrupt(digitalPinToInterrupt(BUTTON_B), buttonB, FALLING);
  attachInterrupt(digitalPinToInterrupt(BUTTON_C), buttonC, FALLING);
  attachInterrupt(digitalPinToInterrupt(BUTTON_D), buttonD, FALLING);

  WiFi.begin(wifiSSID, wifiPassword);
  Serial.printf("Connecting to WiFi network %s...", wifiSSID);

  // Try to connect to WiFi and timeout if not successful after wifiInterval
  currentMillis = millis();
  wifiStartMillis = currentMillis;
  while (WiFi.status() != WL_CONNECTED && currentMillis - wifiStartMillis < wifiInterval) {
    currentMillis = millis();
    if(currentMillis - lastFlashMillis >= flashInterval) {
      if(pixelOn) {
        pixels.setPixelColor(0, pixels.Color(192,128,0));
        pixels.show();
        pixelOn = false;
        Serial.print(".");
      } else {
        pixels.setPixelColor(0, pixels.Color(192,0,0));
        pixels.show();
        pixelOn = true;
        Serial.print(".");
      }
      lastFlashMillis = currentMillis;
    }
  }

  // Check if connection succeeded or timed out
  if(WiFi.status() != WL_CONNECTED) {
    Serial.println("FAILED.");
    Serial.println("Proceeding to main loop with state ERROR.");
    state = States::ERROR;
  } else {
    Serial.println("CONNECTED.");
    Serial.println("Proceeding to main loop with state READY.");
    state = States::READY;
  }
}

// Four event handles for four buttons to trigger the state associated with
// each button. Event handler code should be as short as possible.
// TODO: Remove Serial.println()-s and else clauses for production.
void buttonA() {
  if(state == States::READY) {
    Serial.println("Button A pressed. Setting state to CLOSE.");
    state = States::CLOSE;
  } else {
    Serial.println("Button A pressed but not in READY state. No action.");
  }
}
void buttonB() {
  if(state == States::READY) {
    Serial.println("Button B pressed. Setting state to SOFTOPEN.");
    state = States::SOFTOPEN;
  } else {
    Serial.println("Button B pressed but not in READY state. No action.");
  }
}
void buttonC() {
  if(state == States::READY) {
    Serial.println("Button C pressed. Setting state to OPEN.");
    state = States::OPEN;
  } else {
    Serial.println("Button C pressed but not in READY state. No action.");
  }
}
void buttonD() {
  if(state == States::READY) {
    Serial.println("Button D pressed. Setting state to CLEAR.");
    state = States::CLEAR;
  } else {
    Serial.println("Button D pressed but not in READY state. No action.");
  }
}

HTTPClient http;
int httpCode;

void loop() {
  // We grab the current clock time every time through the loop. Delays and flashes
  // are all calculated to avoid blocking code in the loop.
  currentMillis = millis();

  // FSM
  switch (state) {
    case States::BOOT:
      // For my OCD, although this state should never happen after setup()
      break;
    case States::READY:
      // Reaady for input, purple steady
      // pixels.clear();  // Maybe off instead?
      pixels.setPixelColor(0,pixels.Color(60,0,128));
      pixels.show();
      pixelOn = true;
      break;
    case States::CLOSE:
      waitStartMillis = currentMillis;
      lastFlashMillis = currentMillis;
      Serial.println("CLOSE state triggered. Calling CLOSE webhook.");

      // HTTPS call to Airtable webhook
      http.begin(urlShopStatus);
      http.addHeader("Content-Type", "application/json");
      httpCode = http.POST(closeShopJSON);
      if (httpCode > 0) {
        if (httpCode == HTTP_CODE_OK) {
          String payload = http.getString();
          Serial.println(payload);
        } else {
          Serial.printf("[HTTP] POST... code: %d\n", httpCode);
        }
      } else {
        Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
      }
      http.end();
      // END HTTPS call to Airtable webhook

      Serial.println("Starting Post-Close wait timer.");
      state = States::CLOSEWAIT; // Switching to Post-CLose wait state
      break;
    case States::CLOSEWAIT:
      // Toggle the LED if the flash interval has passed
      if(currentMillis - lastFlashMillis >= flashInterval) {
        if(pixelOn) {
          pixels.setPixelColor(0, pixels.Color(0,0,0));
          pixels.show();
          pixelOn = false;
        } else {
          pixels.setPixelColor(0, pixels.Color(192,0,0));
          pixels.show();
          pixelOn = true;
        }
        lastFlashMillis = currentMillis;
      }
      // Exit CLOSEWAIT state if the waitInterval has passed
      if(currentMillis - waitStartMillis >= waitInterval) {
        pixels.setPixelColor(0, pixels.Color(0,0,0));
        pixels.show();
        pixelOn = false;
        state = States::READY;
        Serial.println("Done with CLOSEWAIT state. Switching to READY.");
      }
      break;
    case States::SOFTOPEN:
      waitStartMillis = currentMillis;
      lastFlashMillis = currentMillis;
      Serial.println("SOFTOPEN state triggered. Calling SOFTOPEN webhook.");

      // HTTPS call to Airtable webhook
      http.begin(urlShopStatus);
      http.addHeader("Content-Type", "application/json");
      httpCode = http.POST(softopenShopJSON);
      if (httpCode > 0) {
        if (httpCode == HTTP_CODE_OK) {
          String payload = http.getString();
          Serial.println(payload);
        } else {
          Serial.printf("[HTTP] POST... code: %d\n", httpCode);
        }
      } else {
        Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
      }
      http.end();
      // END HTTPS call to Airtable webhook

      Serial.println("Starting Post-Soft Open wait timer.");
      state = States::SOFTOPENWAIT; // Switching to Post-Soft Open wait state
      break;
    case States::SOFTOPENWAIT:
      // Toggle the LED if the flash interval has passed
      if(currentMillis - lastFlashMillis >= flashInterval) {
        if(pixelOn) {
          pixels.setPixelColor(0, pixels.Color(0,0,0));
          pixels.show();
          pixelOn = false;
        } else {
          pixels.setPixelColor(0, pixels.Color(192,128,0));
          pixels.show();
          pixelOn = true;
        }
        lastFlashMillis = currentMillis;
      }
      // Exit SOFTOPENWAIT state if the waitInterval has passed
      if(currentMillis - waitStartMillis >= waitInterval) {
        pixels.setPixelColor(0, pixels.Color(0,0,0));
        pixels.show();
        pixelOn = false;
        state = States::READY;
        Serial.println("Done with SOFTOPENWAIT state. Switching to READY.");
      }
      break;
    case States::OPEN:
      waitStartMillis = currentMillis;
      lastFlashMillis = currentMillis;
      Serial.println("OPEN state triggered. Calling OPEN webhook.");

      // HTTPS call to Airtable webhook
      http.begin(urlShopStatus);
      http.addHeader("Content-Type", "application/json");
      httpCode = http.POST(openShopJSON);
      if (httpCode > 0) {
        if (httpCode == HTTP_CODE_OK) {
          String payload = http.getString();
          Serial.println(payload);
        } else {
          Serial.printf("[HTTP] POST... code: %d\n", httpCode);
        }
      } else {
        Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
      }
      http.end();
      // END HTTPS call to Airtable webhook

      Serial.println("Starting Post-Open wait timer.");
      state = States::OPENWAIT; // Switching to Post-Open wait state
      break;
    case States::OPENWAIT:
      // Toggle the LED if the flash interval has passed
      if(currentMillis - lastFlashMillis >= flashInterval) {
        if(pixelOn) {
          pixels.setPixelColor(0, pixels.Color(0,0,0));
          pixels.show();
          pixelOn = false;
        } else {
          pixels.setPixelColor(0, pixels.Color(0,128,0));
          pixels.show();
          pixelOn = true;
        }
        lastFlashMillis = currentMillis;
      }
      // Exit OPENWAIT state if the waitInterval has passed
      if(currentMillis - waitStartMillis >= waitInterval) {
        pixels.setPixelColor(0, pixels.Color(0,0,0));
        pixels.show();
        pixelOn = false;
        state = States::READY;
        Serial.println("Done with OPENWAIT state. Switching to READY.");
      }
      break;
    case States::CLEAR:
      waitStartMillis = currentMillis;
      lastFlashMillis = currentMillis;
      Serial.println("CLEAR state triggered. Calling CLEAR webhook.");

      // HTTPS call to Airtable webhook
      http.begin(urlClearCheckin);
      http.addHeader("Content-Type", "application/json");
      httpCode = http.POST(clearCheckinJSON);
      if (httpCode > 0) {
        if (httpCode == HTTP_CODE_OK) {
          String payload = http.getString();
          Serial.println(payload);
        } else {
          Serial.printf("[HTTP] POST... code: %d\n", httpCode);
        }
      } else {
        Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
      }
      http.end();
      // END HTTPS call to Airtable webhook

      Serial.println("Starting Post-Clear wait timer.");
      state = States::CLEARWAIT; // Switching to Post-CLear wait state
      break;
    case States::CLEARWAIT:
      // Toggle the LED if the flash interval has passed
      if(currentMillis - lastFlashMillis >= flashInterval) {
        if(pixelOn) {
          pixels.setPixelColor(0, pixels.Color(0,0,0));
          pixels.show();
          pixelOn = false;
        } else {
          pixels.setPixelColor(0, pixels.Color(0,0,255));
          pixels.show();
          pixelOn = true;
        }
        lastFlashMillis = currentMillis;
      }
      // Exit CLEARWAIT state if the waitInterval has passed
      if(currentMillis - waitStartMillis >= waitInterval) {
        pixels.setPixelColor(0, pixels.Color(0,0,0));
        pixels.show();
        pixelOn = false;
        state = States::READY;
        Serial.println("Done with CLEARWAIT state. Switching to READY.");
      }
      break;
    case States::ERROR:
      // We run two timers in this state. We basically wait for wifiBackoff (default 5 minutes)
      // and then try to reconnect for wifiInterval (up to 20 seconds). If we manage to reconnect
      // we go to READY and if we don't manage to reconnect we stay in ERROR. Somewhat complicated
      // but this allows us to regularly try for a reconnect without constantly hitting WiFi if
      // there is a more fundamental problem, such as no network in range.
      //
      // WiFi retry code goes here.
      break;
  }
}
