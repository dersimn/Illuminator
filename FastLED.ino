#define FASTLED_INTERVAL  (1000 / FASTLED_FPS)

NeoPixelBus<NeoGrbFeature, NeoEsp8266Dma800KbpsMethod> strip(FASTLED_NUM_LEDS);
CRGB leds[FASTLED_NUM_LEDS];

Thread ledOutputThread = Thread();
Thread ledAnimationThread = Thread();

uint8_t hue = 0;
uint8_t sat = 0;
uint8_t bri = 0;
uint8_t ani = 0;
String aniStr[] = {"none", "sinelon", "confetti", "colorwheel"};

void setup_FastLED() {
  strip.Begin();
  strip.Show();  

  ledOutputThread.onRun(ledOutput);
  ledOutputThread.setInterval(FASTLED_INTERVAL);
  threadControl.add(&ledOutputThread);

  ledAnimationThread.onRun(ledAnimationController);
  ledAnimationThread.setInterval(FASTLED_INTERVAL);
  threadControl.add(&ledAnimationThread);
}
void setup_FastLED_Network() {
  mqtt.subscribe(s+MQTT_PREFIX+"/set/"+BOARD_ID+"/fastled", light_subscribe);
  publishLight();
}

void light_subscribe(String topic, String message) {
  DynamicJsonBuffer jsonBuffer;
  JsonVariant root = jsonBuffer.parse(message);

  if ( root.is<float>() || root.is<int>() ) {
    ani = 0;
    setBri( root.as<float>() );

    publishLight();
    return;
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
    if ( rootObject.containsKey("animation") ) {
      String recv = rootObject["animation"].as<String>();
      if      (recv == aniStr[0]) { ani = 0; }
      else if (recv == aniStr[1]) { ani = 1; }
      else if (recv == aniStr[2]) { ani = 2; }
      else if (recv == aniStr[3]) { ani = 3; }
      
      publishLight();
      return;
    } else {
      ani = 0;
    }
    publishLight();
  }
}
void publishLight() {
  String output;
  DynamicJsonBuffer jsonBuffer;

  JsonObject& root = jsonBuffer.createObject();
  root["val"] = rescale(bri, 255, 1.0);
  if (ani) {
    root["animation"] = aniStr[ani];
  } else {
    root["hue"] = rescale(hue, 255, 1.0);
    root["sat"] = rescale(sat, 255, 1.0);
  }

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

void ledAnimationController() {
  switch(ani) {
    case 1:
      hue++;
      ledAnimationLoop_Sinelon(13);
      break;
    case 2:
      hue++;
      ledAnimationLoop_Confetti();
      break;
    case 3:
      hue++;
      ledSolidColor(hue, sat, bri);
      break;
    case 0:
    default:
      fill_solid(leds, FASTLED_NUM_LEDS, CHSV(hue, sat, bri));
  }
}

void ledOutput() { 
  RgbColor pixel;
  for (int i = 0; i < FASTLED_NUM_LEDS; i++) {
    pixel = RgbColor(leds[i].r, leds[i].g, leds[i].b);
    strip.SetPixelColor(i, pixel);
  }
  strip.Show();  
}

void ledSolidColor(uint8_t hue, uint8_t sat, uint8_t bri) {
  fill_solid(leds, FASTLED_NUM_LEDS, CHSV(hue, sat, bri));
}
void ledAnimationLoop_Sinelon(uint8_t bpm) {
  fadeToBlackBy(leds, FASTLED_NUM_LEDS, FASTLED_INTERVAL);
  uint16_t pos = beatsin16(bpm, 0, FASTLED_NUM_LEDS-1);
  leds[pos] += CHSV(hue, 255, bri);
}
void ledAnimationLoop_Confetti() {
  fadeToBlackBy(leds, FASTLED_NUM_LEDS, FASTLED_INTERVAL/2);
  uint16_t pos = random16(FASTLED_NUM_LEDS);
  leds[pos] += CHSV(hue+random8(64), 255, bri);
}

