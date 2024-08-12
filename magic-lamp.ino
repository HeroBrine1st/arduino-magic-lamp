#define DEBUG 1

#define HC_ECHO 2       // пин Echo
#define HC_TRIG 3       // пин Trig


#define LED_MAX_MA 2000 // ограничение тока ленты, ма
#define LED_PIN 13      // пин ленты
#define LED_NUM 51      // к-во светодиодов
#define PULSE_PIN 4

#define VB_DEB 0        // отключаем антидребезг (он есть у фильтра)
#define VB_CLICK 500    // таймаут клика
#include <VirtualButton.h>
VButton gest;

#include <FastLED.h>
CRGB leds[LED_NUM];

// структура настроек
struct Data {
  bool stateDeprecated = 1;     
  byte mode = 0;      // 0 цвет, 1 теплота, 2 огонь
  byte bright[3] = {30, 30, 30};  // яркость
  uint16_t value[3] = {0, 0, 0};      // параметр эффекта (цвет...)
};

Data data;

// менеджер памяти
#include <EEManager.h>
EEManager mem(data);

int prev_br;
bool isEnabled = true;

void setup() {
  #ifdef DEBUG
    Serial.begin(115200);
  #endif

  pinMode(HC_TRIG, OUTPUT); // trig выход
  pinMode(HC_ECHO, INPUT);  // echo вход
  pinMode(PULSE_PIN, OUTPUT);

  // FastLED
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, LED_NUM);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, LED_MAX_MA);
  

  mem.begin(0, 'a');  // запуск и чтение настроек
}

void loop() {
  mem.tick();   // менеджер памяти
  
  if(
    updateBrightnessTask() | // NOT A BUG, short circuit is unwanted
    renderEffect() 
  ) FastLED.show(); 


  static uint32_t tmr;
  // 20 герц если используем расстояние, иначе 50
  int period = gest.hold() ? 50 : 20;
  if (millis() - tmr >= period) {
    tmr = millis();
    

    static uint32_t tout;   // таймаут настройки (удержание)
    static int offset_d;    // оффсеты для настроек
    static uint16_t offset_v;
    static bool was_hold = false;

    int dist = getDist(HC_TRIG, HC_ECHO); // получаем расстояние
    dist = getFilterMedian(dist);         // медиана
    dist = getFilterSkip(dist);           // пропускающий фильтр
    int dist_f = getFilterExp(dist);      // усреднение

    gest.poll(dist);                      // расстояние > 0 - это клик
    digitalWrite(PULSE_PIN, dist > 0);

    #ifdef DEBUG
      if(dist_f > 0) {
        Serial.print("[Gestures task] dist_f=");
        Serial.print(dist_f);
        Serial.print(" dist=");
        Serial.print(dist);
        Serial.print(" isEnabled=");
        Serial.println(isEnabled);
      }
    #endif

    // есть клики и прошло 2 секунды после настройки (удержание)
    if (gest.hasClicks() && millis() - tout > 2000) {
      #ifdef DEBUG
        Serial.print("[Gestures task] ");
        Serial.print(gest.clicks);
        Serial.println(" clicks");
      #endif
      switch (gest.clicks) {
        case 1:
          isEnabled = !isEnabled;
          mem.update();
          break;
        case 2:
          if(isEnabled) {
            if(++data.mode >= 3) data.mode = 0;
            mem.update();
          }
      }
    }

    // TODO enable back when async pulse is possible
    // // клик
    // if (gest.click() && data.state) {
    //   Serial.println("Click");
    //   pulse();  // мигнуть яркостью
    // }

    // удержание (выполнится однократно)
    if (gest.held() && isEnabled) {
      #ifdef DEBUG
        Serial.println("[Gestures task] Held");
      #endif
      pulse();  // мигнуть яркостью
      offset_d = dist_f;    // оффсет расстояния для дальнейшей настройки
      switch (gest.clicks) {
        case 0: offset_v = data.bright[data.mode]; break;   // оффсет яркости
        case 1: offset_v = data.value[data.mode]; break;    // оффсет значения
      }
    }

    // удержание (выполнится пока удерживается)
    if (gest.hold() && isEnabled) {
      tout = millis();
      if(!was_hold) {
        was_hold = true;
      }
      // смещение текущей настройки как оффсет + (текущее расстояние - расстояние начала)
      int val = offset_v + (dist_f - offset_d);
      if(gest.clicks == 1) {
        switch(data.mode) {
          case 0:
          case 2:
            val = val % 766 + ((val < 0) ? 766 : 0);
            break;
          case 1:
            val = constrain(val, 0, 1428);
        }
      } else {
        val = constrain(val, 0, 255);
      }
      
      // применяем
      switch (gest.clicks) {
        case 0: 
          data.bright[data.mode] = val; 
          break;
        case 1: 
          data.value[data.mode] = val; 
          break;
      }
      
      mem.update();
    } else if(was_hold) {
      was_hold = false;
    }
  }
}

// получение расстояния с дальномера
#define HC_MAX_LEN 1000L  // макс. расстояние измерения, мм
int getDist(byte trig, byte echo) {
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  // измеряем время ответного импульса
  uint32_t us = pulseIn(echo, HIGH, (HC_MAX_LEN * 2 * 1000 / 343));

  // считаем расстояние и возвращаем
  return (us * 343L / 2000);
}

// медианный фильтр
int getFilterMedian(int newVal) {
  static int buf[3];
  static byte count = 0;
  buf[count] = newVal;
  if (++count >= 3) count = 0;
  return (max(buf[0], buf[1]) == max(buf[1], buf[2])) ? max(buf[0], buf[2]) : max(buf[1], min(buf[0], buf[2]));
}

// пропускающий фильтр
#define FS_WINDOW 7   // количество измерений, в течение которого значение не будет меняться
#define FS_DIFF 80    // разница измерений, с которой начинается пропуск
int getFilterSkip(int val) {
  static int prev;
  static byte count;

  if (!prev && val) prev = val;   // предыдущее значение 0, а текущее нет. Обновляем предыдущее
  // позволит фильтру резко срабатывать на появление руки

  // разница больше указанной ИЛИ значение равно 0 (цель пропала)
  if (abs(prev - val) > FS_DIFF || !val) {
    count++;
    // счётчик потенциально неправильных измерений
    if (count > FS_WINDOW) {
      prev = val;
      count = 0;
    } else val = prev;
  } else count = 0;   // сброс счётчика
  prev = val;
  
  return val;
}

// экспоненциальный фильтр со сбросом снизу
#define ES_EXP 2L     // коэффициент плавности (больше - плавнее)
#define ES_MULT 16L   // мультипликатор повышения разрешения фильтра
int getFilterExp(int val) {
  static long filt;
  if (val) filt += (val * ES_MULT - filt) / ES_EXP;
  else filt = 0;  // если значение 0 - фильтр резко сбрасывается в 0
  // в нашем случае - чтобы применить заданную установку и не менять её вниз к нулю
  return filt / ES_MULT;
}



bool renderEffect() {
  static uint8_t prevMode = 255; 

  if(FastLED.getBrightness() == 0) return; // state is off and no animation is ongoing

  bool needsOutput = false;

  switch(data.mode) {
    case 0:
      needsOutput = hueEffect(data.value[0] / 3, prevMode != 0);
      prevMode = 0;
      break;
    case 1:
      needsOutput = kelvinEffect(data.value[1] * 28, prevMode != 1);
      prevMode = 1;
      break;
    case 2:
      needsOutput = fireEffect(data.value[2] / 3, prevMode != 2);
      prevMode = 2;
      break;
  }

  return needsOutput;
}

#define BRIGHTNESS_FRAME_TIME_MS 10
#define PULSE_BRIGHTNESS_REDUCTION 45
#define PULSE_FRAME_REDUCTION 3
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
        // Gradually lower (or brighten, depending on current brightness) current brightness by PULSE_BRIGHTNESS_REDUCTION
        // INITIAL REQUIREMENTS
        // animCache is set to zero
        if(animCache < PULSE_BRIGHTNESS_REDUCTION) {
          animCache += PULSE_FRAME_REDUCTION;
        } else animMode = 2;
      } else if(animMode == 2) {
        // PULSE ANIMATION EFFECT
        // Gradually return brightness back
        // INITIAL REQUIREMENTS
        // animCache is set to PULSE_BRIGHTNESS_REDUCTION
        if(animCache > 0) {
          animCache -= PULSE_FRAME_REDUCTION;
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
      if(currentBrightness <= 255 - PULSE_BRIGHTNESS_REDUCTION) currentBrightness += animCache;
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
    currentBrightness = ((uint32_t)(currentBrightness + 1) * currentBrightness) >> 8;
    if(isEnabled) currentBrightness = max(currentBrightness, 2);
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