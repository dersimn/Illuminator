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

int currentStep = 0;
CHSV oldColor = CHSV(0,0,0);

void setup_FastLED() {
  strip.Begin();
  strip.Show();  

  ledOutputThread.onRun(ledOutput);
  ledOutputThread.setInterval(FASTLED_INTERVAL);
  threadControl.add(&ledOutputThread);
}
void setup_FastLED_Network() {
  mqtt.subscribe(s+APP_PREFIX+"/set/"+ESP_ID+"/fastled", light_subscribe);
  publishLight();
}

void light_subscribe(String topic, String message) {
  StaticJsonDocument<500> doc;
  auto error = deserializeJson(doc, message);
  if (error) {
    Log.error(s+"deserializeJson() failed with code "+error.c_str());
    return;
  }

  if ( doc.is<bool>() ) {                                 // bool
    ani = 0;
    setBri( doc.as<bool>() ? 1.0 : 0.0 );
  } else if ( doc.is<float>() || doc.is<int>() ) {        // int / float
    ani = 0;
    setBri( doc.as<float>() );
  } else if ( doc.is<JsonObject>() ) {                    // object
    JsonObject rootObject = doc.as<JsonObject>();
    if ( rootObject.containsKey("val") ) {
      setBri( rootObject["val"] );
    }
    if ( rootObject.containsKey("hue") ) {
      setHue( rootObject["hue"] );
    }
    if ( rootObject.containsKey("sat") ) {
      setSat( rootObject["sat"] );
    }
    
    transitionTime = rootObject["transitiontime"] | 400;
    
    String recv = rootObject["animation"] | "none";
    if      (recv == aniStr[0]) { ani = 0; }
    else if (recv == aniStr[1]) { ani = 1; }
    else if (recv == aniStr[2]) { ani = 2; }
    else if (recv == aniStr[3]) { ani = 3; }
  }

  if (currentStep) {
    currentStep = 0;
    oldColor = CHSV(hue, sat, bri); 
  }
  publishLight();
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

  mqtt.publish(s+APP_PREFIX+"/status/"+ESP_ID+"/fastled", doc.as<String>(), true);
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
  if (oldColor == CHSV(hue, sat, bri)) {
    fill_solid(leds, FASTLED_NUM_LEDS, CHSV(hue, sat, bri));
  } else {
    int stepsNeededForTransition = limit(transitionTime / FASTLED_INTERVAL, 1, 255);

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
