void gesturesTask() {
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
// Changing is not recommended, this function already wastes 5 ms of void loop
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
