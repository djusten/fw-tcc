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
#include <PubSubClient.h>
#include <MQTT.h>
#include "EEPROMAnything.h"

#define EEPROM_MAX_ADDRS 512

enum {
  PROG_INIT,
  PROG_RUN,
  PROG_MQTT,
  PROG_CONFIG
} prog_status;

config_t config;

WiFiClient wclient;
PubSubClient clientMQTT(wclient, config.broker);

int state;
String humidTopic;

bool testWifi(void) {
  int c = 0;

  Serial.println("\nTesting WiFi...");

  while ( c < 20 ) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("");
      Serial.println("WiFi connected");
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());
      return true;
    }
    delay(500);
    //Serial.print(WiFi.status());
    Serial.print(".");
    c++;
  }
  Serial.println("\nConnect timed out, opening AP");
  return false;
}

bool saveConfig(int confirmation, char *broker, char *topicHumidity,
                char *wifiSsid, char *wifiPass, char *mqttUser, char *mqttPassword)
{
  config.confirmation = confirmation;
  strncpy(config.broker, broker, sizeof(config.broker));
  strncpy(config.topicHumidity, topicHumidity, sizeof(config.topicHumidity));
  strncpy(config.wifiSsid, wifiSsid, sizeof(config.wifiSsid));
  strncpy(config.wifiPass, wifiPass, sizeof(config.wifiPass));
  strncpy(config.mqttUser, mqttUser, sizeof(config.mqttUser));
  strncpy(config.mqttPassword, mqttPassword, sizeof(config.mqttPassword));
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
  Serial.print("mqttUser is: ");
  Serial.println(config.mqttUser);
  Serial.print("mqttPass is: ");
  Serial.println(config.mqttPassword);

  return true;
}

void setup_wifi() {

  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(config.wifiSsid);

  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.begin(config.wifiSsid, config.wifiPass);
}

void setup()
{
  Serial.begin(115200);

  EEPROM.begin(EEPROM_MAX_ADDRS);

  state = PROG_INIT;

  saveConfig(41, "iot.eclipse.org", "onionOut", "cebola", "cebola", "onion", "onion");
  loadConfig();

  setup_wifi();
}

String macToStr(const uint8_t* mac) {
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
    if (i < 5)
      result += ':';
  }
  return result;
}

bool config_mqtt()
{
  clientMQTT = PubSubClient(wclient, config.broker);

  uint8_t mac[6];
  WiFi.macAddress(mac);
  String clientID = "esp_" + macToStr(mac);

  if (clientMQTT.connect(MQTT::Connect(clientID).set_auth(config.mqttUser, config.mqttPassword))) {
    Serial.println("connected to MQTT broker!");
    humidTopic = String(config.topicHumidity);
    return true;
  }
  return false;
}

void loop()
{
  if (state == PROG_INIT) {
    if (testWifi()) {
      Serial.println("Could connect");
      state = PROG_MQTT;
    } else {
      Serial.println("Could NOT connect");
      state = PROG_CONFIG;
    }
  }
  else if (state == PROG_MQTT) {
    if (config_mqtt()) {
      state = PROG_RUN;
    }
    else {
      state = PROG_CONFIG;
    }
  }
  else if (state == PROG_CONFIG) {
    Serial.println("Start web");
    delay(5000);
  }
  else if (state == PROG_RUN) {
    String temperature = String(5, 1);
    Serial.println("Onion bugs =) | running");
    clientMQTT.loop();
    MQTT::Publish pub(humidTopic, temperature);
    pub.set_retain(true);
    clientMQTT.publish(pub);
    delay(5000);
    //ESP.deepSleep(sleepTimeS * 1000000);
  }
}
