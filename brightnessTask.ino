#define BRIGHTNESS_FRAME_TIME_MS 10
#define PULSE_TOTAL_BRIGHTNESS_REDUCTION 45
#define PULSE_ONE_FRAME_REDUCTION 3 
#define BRIGHTNESS_CROSSFADE_STEP 4

bool pulseNeeded = false;
bool updateBrightnessTask() {
  static uint8_t prevBrightness = 0,
    // 0 - no animation
    // 1 - reducing brightness
    // 2 - increasing brightness back
    // 3 - crossfade between effects
    animMode = 0,
    // internal cache of current animation mode
    animCache = 0,
    // previous effect
    prevMode = 255;

  static bool wasEnabled = false;
  uint8_t currentBrightness = 0;

  if(wasEnabled != isEnabled) {
    wasEnabled = isEnabled;
    animCache = prevBrightness;
    animMode = 3;
    prevMode = data.mode;
    #ifdef DEBUG
      Serial.print("[Brightness task] Changing state. isEnabled=");
      Serial.print(isEnabled);
      Serial.print(", animCache=");
      Serial.print(animCache);
      Serial.print(", animMode=");
      Serial.print(animMode);
      Serial.print(", prevMode=");
      Serial.println(prevMode);
    #endif
  }

  if(isEnabled) {
    currentBrightness = data.bright[data.mode];

    if(animMode == 0) {
      if(pulseNeeded) {
        pulseNeeded = false;
        animMode = 1;
        animCache = 0;
      } else if (prevMode != data.mode) {
        animMode = 3;
        if(prevMode == 255) animCache = 0;
        else animCache = data.bright[prevMode];
        prevMode = data.mode;
      }
    }
  }

  if(animMode != 0) {
    static uint32_t lastFrameMs = 0;

    if(millis() - lastFrameMs >= BRIGHTNESS_FRAME_TIME_MS) {
      lastFrameMs = millis();

      if(animMode == 1) {
        // PULSE ANIMATION EFFECT
        // Gradually lower (or brighten, depending on current brightness) current brightness by PULSE_TOTAL_BRIGHTNESS_REDUCTION
        // INITIAL REQUIREMENTS
        // animCache is set to zero
        if(animCache < PULSE_TOTAL_BRIGHTNESS_REDUCTION) {
          animCache += PULSE_ONE_FRAME_REDUCTION;
        } else animMode = 2;
      } else if(animMode == 2) {
        // PULSE ANIMATION EFFECT
        // Gradually return brightness back
        // INITIAL REQUIREMENTS
        // animCache is set to PULSE_TOTAL_BRIGHTNESS_REDUCTION
        if(animCache > 0) {
          animCache -= PULSE_ONE_FRAME_REDUCTION;
        } else animMode = 0;
      }
      else if(animMode == 3) {
        // BRIGHTNESS ANIMATION EFFECT
        // Gradually increase (or decrease)_brightness to match enabled mode
        // INITIAL REQUIREMENTS
        // animCache is set to brightness of previous mode (or 0 if was disabled)

        if(abs(animCache - currentBrightness) > BRIGHTNESS_CROSSFADE_STEP) {
          int shift = animCache > currentBrightness ? -BRIGHTNESS_CROSSFADE_STEP : BRIGHTNESS_CROSSFADE_STEP;
          animCache += shift;
          #ifdef DEBUG
            // yes, they're swapped. No, it's not an error. This print does not lie:
            // currentBrightness is actually brightness of effect, while animCache holds brightness already shown to LED
            Serial.print("[Brightness task] Brightness crossfade tick. target=");
            Serial.print(currentBrightness);
            Serial.print(", currentBrightness=");
            Serial.println(animCache);
          #endif
        } else {
          #ifdef DEBUG
            // See commend above
            Serial.print("[Brightness task] Brightness crossfade complete (target=");
            Serial.print(currentBrightness);
            Serial.print(", currentBrightness=");
            Serial.print(animCache);
            Serial.println(")");
          #endif
          animMode = 0;
        }
      }
    }

    // Update brightness (no millis guard)
    if(animMode == 1 || animMode == 2) {
      if(currentBrightness <= 255 - PULSE_TOTAL_BRIGHTNESS_REDUCTION) currentBrightness += animCache;
      else currentBrightness -= animCache;
    } else if(animMode == 3) {
      currentBrightness = animCache;
    }
  }

  if(prevBrightness != currentBrightness) {
    prevBrightness = currentBrightness;
    #ifdef DEBUG
      Serial.print("[Brightness task] Settings brightness to raw=");
      Serial.print(currentBrightness);
    #endif
    currentBrightness = ((uint32_t)(currentBrightness + 1) * currentBrightness) >> 8; // square gamma for human eye
    if(isEnabled) currentBrightness = max(currentBrightness, 2); // disable complete blackout
    #ifdef DEBUG
      Serial.print(", actual=");
      Serial.print(currentBrightness);
      Serial.println();
    #endif
    FastLED.setBrightness(currentBrightness);
    
    return true;
  }
  return false;
}

// подмигнуть яркостью
void pulse() {
  pulseNeeded = true;
}