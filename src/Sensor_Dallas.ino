Thread dallasMeassureThread = Thread();
ThreadRunOnce dallasOutputThread = ThreadRunOnce();

OneWire oneWire(DS_ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

DeviceAddress devices[DS_MAX_DEVICES];
String devices_str[DS_MAX_DEVICES];
uint8_t device_count;

void setup_Sensor_Dallas() {
  LogDallas.info("Detecting Temperature ICs");

#if DS_PARASITE_POWER
  pinMode(DS_ONE_WIRE_BUS, OUTPUT);
#endif

  sensors.begin();
  device_count = sensors.getDeviceCount();

  // Debug out
  if ( device_count != 0 ) {
    LogDallas.info(s+"Found " + device_count + " devices");
  } else {
    LogDallas.warn("No devices found");
  }
  LogDallas.info(s+"Parasite power is: " + ((sensors.isParasitePowerMode()) ? "ON" : "OFF")); 

  // by index
  for (uint8_t i = 0; i < device_count; i++) {
    if (!sensors.getAddress(devices[i], i)) {
      LogDallas.error(s+"Unable to find address for Device " + i);
    }
  }

  // show the addresses we found on the bus
  for (uint8_t i = 0; i < device_count; i++) {
    devices_str[i] = stringPrintAddress(devices[i]);
    LogDallas.info(s+"Device " + i + " Address: " + devices_str[i]);
  }

  // set the resolution per device
  for (uint8_t i = 0; i < device_count; i++) {
    sensors.setResolution(devices[i], DS_PRECISION);
  }

  // Non-blocking temperature reads
  sensors.setWaitForConversion(false);

  dallasMeassureThread.onRun([](){
    sensors.requestTemperatures();
    dallasOutputThread.setRunOnce(5000);    
  });
  dallasMeassureThread.setInterval(DS_INTERVAL);
  threadControl.add(&dallasMeassureThread);

  dallasOutputThread.onRun([](){
    String output;
    StaticJsonDocument<500> doc;

    for (uint8_t i = 0; i < device_count; i++) {
      float tempC = sensors.getTempC(devices[i]);
      if (tempC == -127) {
        LogDallas.warn(s+"Sensor "+devices_str[i]+" read error");
      } else {
        if (devices_str[i] == DS_INTERNAL) {
          mqtt.publish(s+MQTT_PREFIX+"/maintenance/temperature", String(tempC, 2), DS_POST_RETAINED);
        } else {
          doc[devices_str[i]] = tempC;
        }
      }
    }
    mqtt.publish(s+MQTT_PREFIX+"/status/temperature/dallas", doc.as<String>(), DS_POST_RETAINED);
  });
  dallasOutputThread.enabled = false;
  threadControl.add(&dallasOutputThread);
}

String stringPrintAddress(DeviceAddress deviceAddress) {
  String tmp;
  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    if (deviceAddress[i] < 16) tmp += String("0");
    tmp += String(deviceAddress[i], HEX);
  }
  return tmp;
}
