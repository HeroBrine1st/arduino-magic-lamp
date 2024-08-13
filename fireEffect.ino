#define HUE_GAP 21      // заброс по hue
#define FIRE_STEP 15    // шаг огня
#define MIN_BRIGHT 70   // мин. яркость огня
#define MAX_BRIGHT 255  // макс. яркость огня
#define MIN_SAT 245     // мин. насыщенность
#define MAX_SAT 255     // макс. насыщенность

// возвращает цвет огня для одного пикселя
CHSV getFireColor(uint8_t val, uint8_t hue_start) {
  // чем больше val, тем сильнее сдвигается цвет, падает насыщеность и растёт яркость
  return CHSV(
    hue_start + map(val, 0, 255, 0, HUE_GAP),
    // TODO constrain is not necessary
    constrain(map(val, 0, 255, MAX_SAT, MIN_SAT), 0, 255),
    constrain(map(val, 0, 255, MIN_BRIGHT, MAX_BRIGHT), 0, 255)
  );
}

// огненный эффект
bool fireEffect(uint8_t value, bool forceRender) {
  static uint32_t prevTime;
  static int counter;

  if(forceRender) {
    #ifdef DEBUG
      Serial.println("[FireEffect] Forced render");
    #endif
  }

  // двигаем пламя
  if (millis() - prevTime > 20) {
    prevTime = millis();
    counter += 20;
    forceRender = true;
  }

  if(forceRender) {
    for(int i = 0; i < LED_NUM; i++) {
      leds[i] = getFireColor(inoise8(i * FIRE_STEP, counter), value);
    }
  }


  return forceRender;
}