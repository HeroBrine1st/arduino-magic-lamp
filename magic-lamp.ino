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
  byte mode = 0;      // 0 цвет, 1 теплота, 2 огонь
  byte bright[3] = {30, 30, 30};  // яркость
  uint16_t value[3] = {0, 0, 0};      // параметр эффекта (цвет...)
};

Data data;

// менеджер памяти
#include <EEManager.h>
EEManager mem(data);

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

  gesturesTask();
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
