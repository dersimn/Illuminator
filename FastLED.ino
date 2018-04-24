NeoPixelBus<NeoGrbFeature, NeoEsp8266Dma800KbpsMethod> strip(FASTLED_NUM_LEDS);
CRGB leds[FASTLED_NUM_LEDS];

Thread ledOutputThread = Thread();

uint8_t hue = 0;
uint8_t sat = 0;
uint8_t bri = 0;

void setup_FastLED() {
  strip.Begin();
  strip.Show();  

  ledOutputThread.onRun(ledOutput);
  ledOutputThread.setInterval(1000 / FASTLED_FPS);
  threadControl.add(&ledOutputThread);
}
void setup_FastLED_Network() {
  mqtt.subscribe(s+MQTT_PREFIX+"/set/"+BOARD_ID+"/fastled", light_subscribe);
  publishLight();
}

void light_subscribe(String topic, String message) {
  DynamicJsonBuffer jsonBuffer;
  JsonVariant root = jsonBuffer.parse(message);

  if ( root.is<float>() || root.is<int>() ) {
    setBri( root.as<float>() );
  }
  if ( root.is<JsonObject>() ) {
    JsonObject& rootObject = root.as<JsonObject>();
    if ( rootObject.containsKey("val") ) {
      setBri( rootObject["val"].as<float>() );
    }
    if ( rootObject.containsKey("hue") ) {
      setHue( rootObject["hue"].as<float>() );
    }
    if ( rootObject.containsKey("sat") ) {
      setSat( rootObject["sat"].as<float>() );
    }
  }

  ledSolidColor();
  publishLight();
}
void publishLight() {
  String output;
  DynamicJsonBuffer jsonBuffer;

  JsonObject& root = jsonBuffer.createObject();
  root["val"] = rescale(bri, 255, 1.0);
  root["hue"] = rescale(hue, 255, 1.0);
  root["sat"] = rescale(sat, 255, 1.0);

  root.printTo(output);
  mqtt.publish(s+MQTT_PREFIX+"/status/"+BOARD_ID+"/fastled", output, true);
}

float setHue(float val) {
  if ( inRange(val, 0.0, 1.0) ) {
    hue = rescale(val, 1.0, 255);
  }
  return hue;
}
float setSat(float val) {
  if ( inRange(val, 0.0, 1.0) ) {
    sat = rescale(val, 1.0, 255);
  }
  return sat;
}
float setBri(float val) {
  if ( inRange(val, 0.0, 1.0) ) {
    bri = rescale(val, 1.0, 255);
  }
  return bri;
}

void ledOutput() { 
  RgbColor pixel;
  for (int i = 0; i < FASTLED_NUM_LEDS; i++) {
    pixel = RgbColor(leds[i].r, leds[i].g, leds[i].b);
    strip.SetPixelColor(i, pixel);
  }
  strip.Show();  
}

void ledSolidColor() {
  fill_solid(leds, FASTLED_NUM_LEDS, CHSV( hue, sat, bri));
}
