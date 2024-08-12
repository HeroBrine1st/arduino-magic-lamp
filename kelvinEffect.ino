bool kelvinEffect(int kelvin, bool forceRender) {
  static int lastKelvin = 0;
  
  if(lastKelvin != kelvin || forceRender) {
    fill_solid(leds, LED_NUM, blackBodyRadiationKelvin(kelvin));
    lastKelvin = kelvin;
    return true;
  }

  return false;
}

// цветовая температура 1000-40000К
CRGB blackBodyRadiationKelvin(int kelvin) {
  uint8_t r, g, b;
  float tmpKelvin, tmpCalc;
  kelvin = constrain(kelvin, 1000, 40000);
  tmpKelvin = kelvin / 100;
  
  // red
  if (tmpKelvin <= 66) r = 255;
  else {
    tmpCalc = tmpKelvin - 60;
    tmpCalc = (float)pow(tmpCalc, -0.1332047592);
    tmpCalc *= (float)329.698727446;
    tmpCalc = constrain(tmpCalc, 0, 255);
    r = tmpCalc;
  }
  
  // green
  if (tmpKelvin <= 66) {
    tmpCalc = tmpKelvin;
    tmpCalc = (float)99.4708025861 * log(tmpCalc) - 161.1195681661;
    tmpCalc = constrain(tmpCalc, 0, 255);
    g = tmpCalc;
  } else {
    tmpCalc = tmpKelvin - 60;
    tmpCalc = (float)pow(tmpCalc, -0.0755148492);
    tmpCalc *= (float)288.1221695283;
    tmpCalc = constrain(tmpCalc, 0, 255);
    g = tmpCalc;
  }
  
  // blue
  if (tmpKelvin >= 66) b = 255;
  else if (tmpKelvin <= 19) b = 0;
  else {
    tmpCalc = tmpKelvin - 10;
    tmpCalc = (float)138.5177312231 * log(tmpCalc) - 305.0447927307;
    tmpCalc = constrain(tmpCalc, 0, 255);
    b = tmpCalc;
  }
  return CRGB(r, g, b);
}