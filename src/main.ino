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
#include <ESP8266WebServer.h>
#include "DNSServer.h"
#include <PubSubClient.h>
#include <MQTT.h>
#include "EEPROMAnything.h"

#define EEPROM_MAX_ADDRS 512
#define CONFIRMATION_NUMBER 42

enum {
  ACCESS_POINT_WEBSERVER
};

enum {
  PROG_INIT,
  PROG_RUN,
  PROG_MQTT,
  PROG_WAIT_WEB,
  PROG_CONFIG
} prog_status;

config_t config;

ESP8266WebServer server(80);

WiFiClient wclient;
PubSubClient clientMQTT(wclient, config.broker);

int state;
String humidTopic;
String st;
String content;
IPAddress apIP(10, 10, 10, 1);
DNSServer dnsServer;
const byte DNS_PORT = 53;
const char* ssid = "esp-config-mode";

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

void setupAccessPoint(void) {
  Serial.println("setting wifi mode");
  WiFi.mode(WIFI_STA);
  Serial.println("disconnecting");
  if (WiFi.status() == WL_CONNECTED) {
    WiFi.disconnect();
  }
  Serial.println("waiting");
  delay(100);
  Serial.println("scanning");
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0)
    Serial.println("no networks found");
  else
  {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*");
      delay(10);
    }
  }
  Serial.println("");
  st = "<ol>";
  for (int i = 0; i < n; ++i)
  {
    // Print SSID and RSSI for each network found
    st += "<li>";
    st += WiFi.SSID(i);
    st += " (";
    st += WiFi.RSSI(i);
    st += ")";
    st += (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*";
    st += "</li>";
  }
  st += "</ol>";
  delay(100);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(ssid);
  launchWeb(ACCESS_POINT_WEBSERVER);
}

void launchWeb(int webservertype) {
  Serial.println("\nWiFi connected");
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("SoftAP IP: ");
  Serial.println(WiFi.softAPIP());
  setupWebServerHandlers(webservertype);
  // Start the server
  server.begin();
  Serial.print("Server type ");
  Serial.print(webservertype);
  Serial.println(" started");
  //Captive Portal
  dnsServer.start(DNS_PORT, "*", apIP);
}

void setupWebServerHandlers(int webservertype) {
  if ( webservertype == ACCESS_POINT_WEBSERVER ) {
    server.on("/", handleDisplayAccessPoints);
    server.on("/setap", handleSetAccessPoint);
    server.onNotFound(handleNotFound);
  }
}

void handleDisplayAccessPoints() {
  IPAddress ip = WiFi.softAPIP();
  String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
  uint8_t mac[6];
  WiFi.macAddress(mac);
  String macStr = macToStr(mac);
  content = "<!DOCTYPE HTML>\n<html>Hello from ";
  content += ssid;
  content += " at ";
  content += ipStr;
  content += " (";
  content += macStr;
  content += ")<p>";
  content += st;
  content += "<p><form method='get' action='setap'>";
  content += "<label>SSID: </label><input name='ssid' length=32><label>Password: </label><input type='password' name='pass' length=64>";
  content += "<p><label>MQTT Broker URL or IP: </label><input name='broker'><p><label>MQTT Humidity Topic: </label><input name='topicHumidity' placeholder='home/bedroom/humidity'><p><label>MQTT User: </label><input name='user'><p><label>MQTT Password: </label><input type='password' name='mqttpass'>";
  content += "<p><input type='submit'></form>";
  content += "<p>We will attempt to connect to the selected AP and broker and reset if successful.";
  content += "<p>Wait a bit and try to subscribe to the selected topic";
  content += "</html>";
  server.send(200, "text/html", content);
}

void handleSetAccessPoint() {
  Serial.println("entered handleSetAccessPoint");
  int httpstatus = 200;
  config.confirmation = CONFIRMATION_NUMBER;
  server.arg("ssid").toCharArray(config.wifiSsid, DEVICE_CONF_ARRAY_LENGHT);
  server.arg("pass").toCharArray(config.wifiPass, DEVICE_CONF_ARRAY_LENGHT);
  server.arg("broker").toCharArray(config.broker, DEVICE_CONF_ARRAY_LENGHT);
  server.arg("topicHumidity").toCharArray(config.topicHumidity, DEVICE_CONF_ARRAY_LENGHT);
  server.arg("user").toCharArray(config.mqttUser, DEVICE_CONF_ARRAY_LENGHT);
  server.arg("mqttpass").toCharArray(config.mqttPassword, DEVICE_CONF_ARRAY_LENGHT);

  
//  saveConfig(int confirmation, char *broker, char *topicHumidity,
//                char *wifiSsid, char *wifiPass, char *mqttUser, char *mqttPassword)

  Serial.print("vai salvar: ");
  Serial.println(config.confirmation);
  Serial.println(config.wifiSsid);
  Serial.println(config.wifiPass);
  Serial.println(config.broker);
  Serial.println(config.topicHumidity);
  Serial.println(config.mqttUser);
  Serial.println(config.mqttPassword);
  if (sizeof(config.wifiSsid) > 0 && sizeof(config.wifiPass) > 0) {
    WiFi.mode(WIFI_AP_STA);
    WiFi.persistent(false);
    WiFi.begin(config.wifiSsid, config.wifiPass);
    if (testWifi()) {
      Serial.println("\nWifi Connection Success!");
      if (sizeof(config.broker) > 0 && sizeof(config.topicHumidity) > 0) {
        //Serial.println("clearing EEPROM...");
        //clearEEPROM();
        Serial.println("writting EEPROM...");
        EEPROMWriteAnything(0, config);
        EEPROM.commit();
        Serial.println("Done! See you soon2");
        delay(3000);
        server.stop();
        state = PROG_INIT;
      } else {
        content = "<!DOCTYPE HTML>\n<html>No broker or topic setted, please try again.</html>";
        Serial.println("Sending 404");
        httpstatus = 404;
      }
    } else {
      Serial.println("Could not connect to this wifi");
      content = "<!DOCTYPE HTML>\n<html>";
      content += "Failed to connect to AP ";
      content += config.wifiSsid;
      content += ", try again.</html>";
    }
  } else {
    Serial.println("SSID or password not set");
    content = "<!DOCTYPE HTML><html>";
    content += "Error, no ssid or password set?</html>";
    //.println("Sending 404");
    httpstatus = 404;
  }
  server.send(httpstatus, "text/html", content);
}

void handleNotFound() {
  content = "File Not Found\n\n";
  content += "URI: ";
  content += server.uri();
  content += "\nMethod: ";
  content += (server.method() == HTTP_GET) ? "GET" : "POST";
  content += "\nArguments: ";
  content += server.args();
  content += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    content += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", content);
}

void clearEEPROM() {
  for (int i = 0; i < EEPROM_MAX_ADDRS; i++) {
    EEPROM.write(i, 0);
  }
  EEPROM.commit();
  delay(100);
}

void loop()
{
  dnsServer.processNextRequest();
  if (state == PROG_INIT) {
    Serial.print("INIT");

    //saveConfig(41, "iot.eclipse.org", "onionOut", "cebola", "cebola", "onion", "onion");
    loadConfig();

    setup_wifi();

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

    setupAccessPoint();
    state = PROG_WAIT_WEB;
    delay(5000);
  }
  else if (state == PROG_RUN) {
    String temperature = String(5, 1);
    Serial.println("Onion bugs =) | running");
    clientMQTT.loop();
    temperature = "test_publisher";
    MQTT::Publish pub(humidTopic, temperature);
    pub.set_retain(true);
    clientMQTT.publish(pub);
    delay(5000);
    //ESP.deepSleep(sleepTimeS * 1000000);
  }
  server.handleClient();
}
