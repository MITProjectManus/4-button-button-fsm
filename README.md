# 4 Button IoT Button (FSM Version)

## Summary

This is a short example program which runs on an Adafruit Feather ESP32-S2 and calls several POST endpoints in Airtable based on button and timer conditions. This example sets the current status of a makerspace or clears the list of checked in makers from the makerspace's checkin session. This code is written for makerspaces at MIT using Airtable to track their current status and their checked in makers.

## Usage

- Four buttons (Red, Yellow, Green, Blue) perform four functions:
-- RED: Request to set the makerspace status to "Closed"
-- YELLOW: Request to set the makerspace status to "Soft Open"
-- GREEN: Request to set the makerspace status to "Open"
-- BLUE: Request to clear the list of checked in users at makerspace, but checking them all out
- Status LED
-- OFF: Indicates ready status; push a button to do something
-- FLASHING: Indicates waiting status; button has been pushed, and you can't to anything else for the next 5 minutes
-- SOLID RED: Indicates error status; network is down or someone's mashing multiple buttons; will re-check in 5 minutes

## FSM

The code tracks 6 status and transitions between them. They are:

| Status         | Description
|================|=================================================================|
| READY          | Waiting for button to perform action                            |
| CLOSE WAIT     | 5 minute lockout timer running after CLOSE action performed     |
| SOFT OPEN WAIT | 5 minute lockout timer running after SOFT OPEN action performed |
| OPEN WAIT      | 5 minute lockout timer running after OPEN action performer      |
| CLEAR WAIT     | 5 minute lockout timer running after CLEAR action performed     |
| ERROR WAIT     | 5 minute wait timer running after ERROR encountered             |