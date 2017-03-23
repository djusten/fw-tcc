/*
 * Copyright (C) 2017 Diogo Justen. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"
#include <ESP8266WiFi.h>
#include "EEPROMAnything.h"

#define EEPROM_MAX_ADDRS 512
config_t config;

bool testWifi(void) {
  int c = 0;
  Serial.println("\nTesting WiFi...");
  while ( c < 20 ) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Wifi connected!");
      return true;
    }
    delay(500);
    Serial.print(WiFi.status());
    c++;
  }
  Serial.println("\nConnect timed out, opening AP");
  return false;
}

bool saveConfig(int confirmation, char *broker, char *topicHumidity,
                char *wifiSsid, char *wifiPass)
{
  config.confirmation = confirmation;
  strncpy(config.broker, broker, sizeof(config.broker));
  strncpy(config.topicHumidity, topicHumidity, sizeof(config.topicHumidity));
  strncpy(config.wifiSsid, wifiSsid, sizeof(config.wifiSsid));
  strncpy(config.wifiPass, wifiPass, sizeof(config.wifiPass));
  EEPROMWriteAnything(0, config);

  return true;
}

bool loadConfig(void)
{
  EEPROMReadAnything(0, config);

  Serial.print("Confirmation number is: ");
  Serial.println(config.confirmation);
  Serial.print("Brokeris : ");
  Serial.println(config.broker);
  Serial.print("topicHumidity is: ");
  Serial.println(config.topicHumidity);
  Serial.print("wifiSsid is: ");
  Serial.println(config.wifiSsid);
  Serial.print("wifiPass is: ");
  Serial.println(config.wifiPass);

  return true;
}

void setup()
{
  Serial.begin(115200);

  EEPROM.begin(EEPROM_MAX_ADDRS);

  loadConfig();

  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.begin(config.wifiSsid, config.wifiPass);
  if (testWifi()) {
    Serial.println("Could connect");
  } else {
    Serial.println("Could NOT connect");
  }
}

void loop()
{
  Serial.println("Onion bugs =)");
  delay(5000);

}
