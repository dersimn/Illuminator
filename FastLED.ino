#define FASTLED_INTERVAL  (1000 / FASTLED_FPS)

NeoPixelBus<NeoGrbFeature, NeoEsp8266Dma800KbpsMethod> strip(FASTLED_NUM_LEDS);
CRGB leds[FASTLED_NUM_LEDS];

Thread ledOutputThread = Thread();
Thread ledAnimationThread = Thread();

uint8_t hue = 0;
uint8_t sat = 0;
uint8_t bri = 0;
uint8_t ani = 0;
int transitionTime = 400;
String aniStr[] = {"none", "sinelon", "confetti", "colorwheel"};

void setup_FastLED() {
  strip.Begin();
  strip.Show();  

  ledOutputThread.onRun(ledOutput);
  ledOutputThread.setInterval(FASTLED_INTERVAL);
  threadControl.add(&ledOutputThread);
}
void setup_FastLED_Network() {
  mqtt.subscribe(s+MQTT_PREFIX+"/set/"+BOARD_ID+"/fastled", light_subscribe);
  publishLight();
}

void light_subscribe(String topic, String message) {
  StaticJsonDocument<500> doc;
  auto error = deserializeJson(doc, message);
  if (error) {
    Log.error(s+"deserializeJson() failed with code "+error.c_str());
    return;
  }

  if ( doc.is<float>() || doc.is<int>() ) {
    ani = 0;
    setBri( doc.as<float>() );

    publishLight();
    return;
  }
  if ( doc.is<bool>() ) {
    ani = 0;
    setBri( doc.as<bool>() ? 1.0 : 0.0 );

    publishLight();
    return;
  }
  if ( doc.is<JsonObject>() ) {
    JsonObject rootObject = doc.as<JsonObject>();
    if ( rootObject.containsKey("val") ) {
      setBri( rootObject["val"].as<float>() );
    }
    if ( rootObject.containsKey("hue") ) {
      setHue( rootObject["hue"].as<float>() );
    }
    if ( rootObject.containsKey("sat") ) {
      setSat( rootObject["sat"].as<float>() );
    }
    if ( rootObject.containsKey("transitiontime") ) {
      transitionTime = rootObject["transitiontime"].as<int>();
    } else {
      transitionTime = 400; // Restore default value if not in object
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
  StaticJsonDocument<500> doc;
  
  doc["val"] = rescale(bri, 255, 1.0);
  if (ani) {
    doc["animation"] = aniStr[ani];
  } else {
    doc["hue"] = rescale(hue, 255, 1.0);
    doc["sat"] = rescale(sat, 255, 1.0);
  }

  mqtt.publish(s+MQTT_PREFIX+"/status/"+BOARD_ID+"/fastled", doc.as<String>(), true);
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
      fill_solid(leds, FASTLED_NUM_LEDS, CHSV(hue, sat, bri));
      break;
    case 0:
    default:
      ledFade();
  }
}

void ledOutput() {
  ledAnimationController();

  // FastLED to NeoPixel Adapter
  RgbColor pixel;
  for (int i = 0; i < FASTLED_NUM_LEDS; i++) {
    pixel = RgbColor(leds[i].r, leds[i].g, leds[i].b);
    strip.SetPixelColor(i, pixel);
  }
  strip.Show();  
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
void ledFade() {
  static CHSV oldColor = CHSV(0,0,0);
  
  if (oldColor == CHSV(hue, sat, bri)) {
    fill_solid(leds, FASTLED_NUM_LEDS, CHSV(hue, sat, bri));
  } else {
    int stepsNeededForTransition = limit(transitionTime / FASTLED_INTERVAL, 1, 255);
    static int currentStep = 0;

    CRGB oldRGB = oldColor;
    CRGB newRGB = CHSV(hue, sat, bri);
    CRGB middle = blend(oldRGB, newRGB, 255 / stepsNeededForTransition * ++currentStep);
    fill_solid(leds, FASTLED_NUM_LEDS, middle);

    if (currentStep >= stepsNeededForTransition) {
      oldColor = CHSV(hue, sat, bri);
      currentStep = 0;
    }
  }
}
