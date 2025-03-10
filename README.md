# 4 Button IoT Button (FSM Version)

## Summary

This is a short example program which runs on an Adafruit Feather ESP32-S2 and calls several POST endpoints in Airtable based on button and timer conditions. This example sets the current status of a makerspace or clears the list of checked in makers from the makerspace's checkin session. This code is written for makerspaces at MIT using Airtable to track their current status and their checked in makers.

## Usage

- Four buttons (Red, Yellow, Green, Blue) perform four functions:
    - RED: Request to set the makerspace status to "Closed"
    - YELLOW: Request to set the makerspace status to "Soft Open"
    - GREEN: Request to set the makerspace status to "Open"
    - BLUE: Request to clear the list of checked in users at makerspace, but checking them all out
- Status LED
    - OFF: Indicates ready status; push a button to do something
    - FLASHING: Indicates waiting status; button has been pushed, and you can't to anything else for the next 5 minutes
    - SOLID RED: Indicates error status; network is down or someone's mashing multiple buttons; will re-check in 5 minutes

## FSM

The code tracks aeveral states and transitions between them based on button interrupts and timers.

| State | Description |
| --- | --- |
| READY          | Waiting for button to perform action                            |
| CLOSE | Close button pushed, call web hook and transit to CLOSE WAIT |
| CLOSE WAIT     | 5 minute lockout timer running after CLOSE action performed     |
| SOFT OPEN      | Soft Open button pushed, call web hook and transit to SOFT OPEN WAIT |
| SOFT OPEN WAIT | 5 minute lockout timer running after SOFT OPEN action performed |
| OPEN           | Open button pushed, call web hook and transit to OPEN WAIT |
| OPEN WAIT      | 5 minute lockout timer running after OPEN action performer      |
| CLEAR          | Clear button pushed, call web hook and transit to CLEAR WAIT |
| CLEAR WAIT     | 5 minute lockout timer running after CLEAR action performed     |
| ERROR          | No WiFi; pause for 5m and try to reconnect for 20s; rinse and repeat |

Why separate wait states rather than just one? The light flashes in the action color during wait based on what button was pushed and what action was performed. Could set color on button push but we may want to do different things for different actions. For example, setting status is not actually expensive, clearing checkin is. We may only want a long timeout for that one.

## Include file for location specific info

This code relies on an include file, `4-button-button.h` which is not tracked in the repo. It contains definitions for variables, web hooks, and payloads that are specific to one or more locations and should not be publicly shared.
(Specifically the web hooks which can be called without authentication and will need to be changed if they become public as they can be abused.) An example `4-button-button.h` file with sensitive info blanked out is included below and as `4-button-button-sample.h`:

```C
#ifndef FOUR_BUTTON_BUTTON_H
#define FOUR_BUTTON_BUTTON_H

const char* urlShopStatus    = "REPLACE_WITH_URL_TO_SET_STATUS_WEBHOOK";
const char* closeShopJSON    = "{\"id\":\"REPLACE_WITH_MAKERSPACE_RECORD_ID\",\"status\":\"Closed\"}";
const char* softopenShopJSON = "{\"id\":\"REPLACE_WITH_MAKERSPACE_RECORD_ID\",\"status\":\"Soft Open\"}";
const char* openShopJSON     = "{\"id\":\"REPLACE_WITH_MAKERSPACE_RECORD_ID\",\"status\":\"Open\"}";

const char* urlClearCheckin  = "REPLACE_WITH_URL_TO_CLEAR_CHECKINS_WEBHOOK";
const char* clearCheckinJSON = "{\"id\":\"REPLACE_WITH_MAKERSPACE_RECORD_ID\",\"reason\":\"418\"}";

const char* wifiSSID = "REPLACE_WITH_WIFI_SSID";
const char* wifiPassword = "REPLACE_WITH_WIFI_PASSWORD";

#endif // FOUR_BUTTON_BUTTON_H

```
