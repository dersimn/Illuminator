#include "default_config.h"
#include "handy_functions.h"

#include <ESP8266WiFi.h>

#include <GenericLog.h>         // https://github.com/dersimn/ArduinoUnifiedLog
#include <NamedLog.h>
#include <LogHandler.h>
#include <LogSerialModule.h>

#include <Thread.h>             // https://github.com/ivanseidel/ArduinoThread
#include <ThreadController.h>
#include <ThreadRunOnce.h>      // https://github.com/dersimn/ArduinoThreadRunOnce

#include <PubSubClient.h>       // https://github.com/knolleary/pubsubclient
#include <PubSubClientTools.h>  // https://github.com/dersimn/ArduinoPubSubClientTools

#include <ArduinoJson.h>

#include <ArduinoOTA.h>

#include <FastLED.h>            // https://github.com/FastLED/FastLED
#include <NeoPixelBus.h>        // https://github.com/Makuna/NeoPixelBus

#include <OneWire.h>
#include <DallasTemperature.h>  // https://github.com/milesburton/Arduino-Temperature-Control-Library

#define APP_PREFIX "Illuminator"
const String s = "";
const String ESP_ID = upperCaseStr(String(ESP.getChipId(), HEX));
const String BOARD_ID = s+APP_PREFIX+"_"+ESP_ID;
const String MQTT_PREFIX = s+"dersimn/"+APP_PREFIX+"/"+ESP_ID;

WiFiClient        espClient;
PubSubClient      mqttClient(MQTT_SERVER, 1883, espClient);
PubSubClientTools mqtt(mqttClient);

LogHandler logHandler;
LogSerialModule serialModule(115200);

GenericLog Log    (logHandler);
NamedLog   LogWiFi(logHandler, "WiFi");
NamedLog   LogMqtt(logHandler, "MQTT");
NamedLog   LogDallas(logHandler, "Dallas");

ThreadController threadControl = ThreadController();
Thread threadMqtt = Thread();
Thread threadUptime = Thread();
ThreadRunOnce threadMqttRunOnce = ThreadRunOnce();
Thread threadState = Thread();

void setup() {
  logHandler.addModule(&serialModule);
  Serial.println();
  Log.info(s+"Initializing "+BOARD_ID);
  Log.info(s+"Git HASH: "+GIT_HASH);
  Log.info(s+"Git Tag/Branch: "+GIT_TAG_OR_BRANCH);
  Log.info(s+"Build timestamp: "+BUILD_TIMESTAMP);

  // -------------------------- App Important --------------------------
  setup_FastLED();

  // -------------------------- MQTT (1) --------------------------
  threadMqttRunOnce.onRun([](){
    if (WiFi.status() == WL_CONNECTED && !mqtt.connected()) {
      mqttReconnect();
    }
  });
  threadControl.add(&threadMqttRunOnce);

  // -------------------------- Wifi --------------------------
  LogWiFi.info(s+"Connecting to SSID: "+WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.softAPdisconnect(true);
  WiFi.hostname(BOARD_ID);

  static WiFiEventHandler gotIpEventHandler = WiFi.onStationModeGotIP([](const WiFiEventStationModeGotIP& event) {
    LogWiFi.info(s+"(Re)connected with IP: "+WiFi.localIP().toString());
    threadMqttRunOnce.setRunOnce(3000);
  });

  static WiFiEventHandler disconnectedEventHandler = WiFi.onStationModeDisconnected([](const WiFiEventStationModeDisconnected& event) {
    LogWiFi.warn("Connection lost");
  });

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  // -------------------------- OTA --------------------------
  ArduinoOTA.setHostname(BOARD_ID.c_str());
  ArduinoOTA.begin();

  // -------------------------- MQTT (2) --------------------------
  if (WiFi.status() == WL_CONNECTED) {
    mqttReconnect();
  }
  threadMqtt.onRun([](){
    static auto lastState = mqtt.connected();

    if (lastState && !mqtt.connected()) {
      LogMqtt(s+"Connection lost");
    }
    if (WiFi.status() == WL_CONNECTED && !mqtt.connected()) {
      mqttReconnect();
    }

    lastState = mqtt.connected();
  });
  threadMqtt.setInterval(MAINTENANCE_INTERVAL);
  threadControl.add(&threadMqtt);

  // -------------------------- Uptime --------------------------
  threadUptime.onRun([](){
    StaticJsonDocument<500> doc;
    static uint64_t longterm_millis;
    static uint32_t last_millis;
    static uint32_t rollover_count;

    uint32_t this_millis = millis();
    longterm_millis += this_millis - last_millis;
    if (last_millis > this_millis) rollover_count++;
    last_millis = this_millis;

    doc["val"] = longterm_millis;
    doc["millis"] = this_millis;
    doc["rollover"] = rollover_count;

    mqtt.publish(s+MQTT_PREFIX+"/maintenance/uptime", doc.as<String>());
  });
  threadUptime.setInterval(MAINTENANCE_INTERVAL);
  threadControl.add(&threadUptime);

  // -------------------------- App --------------------------
  threadState.onRun([](){
    publishLight();
  });
  threadState.setInterval(MAINTENANCE_INTERVAL);
  threadControl.add(&threadState);
  
  setup_FastLED_Network();
  setup_Sensor_Dallas();

  Log.info("Setup done");
}

void loop() {
  threadControl.run();

  ArduinoOTA.handle();
  mqttClient.loop();
}

void mqttReconnect() {
  LogMqtt.info(s+ "Connecting to "+MQTT_SERVER);
  
  if (mqtt.connect(BOARD_ID, s+MQTT_PREFIX+"/online", 0, true, "false")) {
    LogMqtt.info(s+"Connected and (re)subscribed to "+mqtt.resubscribe()+" topic(s)");

    mqtt.publish(s+MQTT_PREFIX+"/online", "true", true);

    // Post static Info Data once every (Re)connect
    // Info
    StaticJsonDocument<500> doc;

    JsonObject board  = doc.createNestedObject("board");
    board["board-id"] = BOARD_ID;
    board["ip-address"] = WiFi.localIP().toString();
    board["wifi-ssid"] = WiFi.SSID();

    JsonObject build  = doc.createNestedObject("build");
    build["git-hash"] = GIT_HASH;
    build["git-tag"] = GIT_TAG_OR_BRANCH;
    build["build-timestamp"] = BUILD_TIMESTAMP;
    
    mqtt.publish(s+MQTT_PREFIX+"/maintenance/info", doc.as<String>(), true);

    // Config
    doc.clear();
    JsonObject maintenance  = doc.createNestedObject("maintenance");
    maintenance["interval"] = MAINTENANCE_INTERVAL;

    JsonObject fastled  = doc.createNestedObject("fastled");
    fastled["led-count"] = FASTLED_NUM_LEDS;
    fastled["fps"] = FASTLED_FPS;
    
    JsonObject sensor_dallas  = doc.createNestedObject("sensor-dallas");
    sensor_dallas["interval"] = DS_INTERVAL;
    sensor_dallas["precision"] = DS_PRECISION;
    sensor_dallas["max-devices"] = DS_MAX_DEVICES;
    sensor_dallas["parasite-power"] = DS_PARASITE_POWER;

    mqtt.publish(s+MQTT_PREFIX+"/maintenance/config", doc.as<String>(), true);

    // Post current state
    publishLight();
  } else {
    LogMqtt.error(s+"Connection failed with rc="+mqttClient.state());
  }
}
