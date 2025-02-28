#ifndef FOUR_BUTTON_BUTTON_H
#define FOUR_BUTTON_BUTTON_H

const char* urlShopStatus    = "REPLACE_WITH_URL_TO_STATUS_WEBHOOK";
const char* closeShopJSON    = "{\"id\":\"REPLACE_WITH_MAKERSPACE_RECORD_ID\",\"status\":\"Closed\"}";
const char* softopenShopJSON = "{\"id\":\"REPLACE_WITH_MAKERSPACE_RECORD_ID\",\"status\":\"Soft Open\"}";
const char* openShopJSON     = "{\"id\":\"REPLACE_WITH_MAKERSPACE_RECORD_ID\",\"status\":\"Open\"}";

const char* urlClearCheckin  = "REPLACE_WITH_URL_TO_CLEAR_CHECKINS_WEBHOOK";
const char* clearCheckinJSON = "{\"id\":\"REPLACE_WITH_MAKERSPACE_RECORD_ID\",\"reason\":\"418\"}";

const char* wifiSSID = "REPLACE_WITH_WIFI_SSID";
const char* wifiPassword = "REPLACE_WITH_WIFI_PASSWORD";

#endif // FOUR_BUTTON_BUTTON_H
