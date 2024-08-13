// the total time of crossfade is (255 / EFFECT_CROSSFADE_FRAME_PROGRESS) * EFFECT_CROSSFADE_FRAME_TIME_MS
#define EFFECT_CROSSFADE_FRAME_TIME_MS 10
#define EFFECT_CROSSFADE_FRAME_PROGRESS 5

bool renderEffect() {
  static uint8_t prevMode = 255, animProgress = 0, animPrevMode = 255;
  static bool animOngoing = false;
  

  if(FastLED.getBrightness() == 0) return; // state is off and no animation is ongoing

  bool needsOutput = false;

  if(data.mode != prevMode && prevMode != 255) {
    animOngoing = true;
    animProgress = 255;
    animPrevMode = prevMode;

    #ifdef DEBUG
      Serial.print("[Effect render task] Mode changed. Enabling animation from ");
      Serial.print(prevMode);
      Serial.print(" to ");
      Serial.print(data.mode);
      Serial.println();
    #endif
  }
 
  // Make sure `buffer` contains last frame from previous effect
  static CRGB buffer[LED_NUM]; // buffer for previous effect
  if(animOngoing) {
    
    // OPTIMIZATION: force render once, then let effect decide
    if(renderEffectInternal(animPrevMode, animProgress == 255)) {
      // If effect is rendered, copy it to buffer
      memcpy(buffer, leds, sizeof(buffer));

      #ifdef DEBUG
        Serial.print("[Effect render task] Got new frame from previous effect. animProgress=");
        Serial.print(animProgress);
        Serial.println();
      #endif
    }
    // otherwise, buffer stays the same
  }

  // It is guaranteed that prevMode != data.mode is true once animOngoing is true
  // because animOngoing is true only if data.mode != prevMode
  needsOutput = renderEffectInternal(data.mode, prevMode != data.mode);
  if(data.mode != prevMode) {
    prevMode = data.mode;
  }

  
  if(animOngoing) {
    static CRGB buffer2[LED_NUM]; // buffer for current effect
    // If effect is rendered, copy it into buffer
    if(needsOutput) { 
      memcpy(buffer2, leds, sizeof(buffer));
      #ifdef DEBUG
        Serial.print("[Effect render task] Got new frame from current effect. animProgress=");
        Serial.print(animProgress);
        Serial.println();
      #endif
    }

    static uint32_t timer = 0;
    if(millis() - timer >= EFFECT_CROSSFADE_FRAME_TIME_MS) {
      timer = millis();
      // Restore current effect's last frame if needed
      if(!needsOutput) memcpy(leds, buffer2, sizeof(buffer));

      if(animProgress <= EFFECT_CROSSFADE_FRAME_PROGRESS) {
        animOngoing = false;
        animProgress = 0;
      } else {
        animProgress -= EFFECT_CROSSFADE_FRAME_PROGRESS;
      }

      // At this line, `leds` contains last frame from current effect
      nblend(leds, buffer, LED_NUM, animProgress);
      needsOutput = true;
      #ifdef DEBUG
        if(animOngoing) leds[0] = CRGB::White;
        else leds[0] = CRGB::Black;
      #endif


      #ifdef DEBUG
        Serial.print("[Effect render task] Blending effects together. animProgress=");
        Serial.print(animProgress);
        Serial.println();
      #endif
    }
  }

  return needsOutput;
}

bool renderEffectInternal(uint8_t mode, bool force) {
  bool needsOutput;
  switch(mode) {
    case 0:
      needsOutput = hueEffect(data.value[0], force);
      break;
    case 1:
      needsOutput = kelvinEffect(data.value[1] * 28, force);
      break;
    case 2:
      needsOutput = fireEffect(data.value[2], force);
      break;
  }

  return needsOutput;
}