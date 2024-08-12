bool hueEffect(uint8_t hue, bool forceRender) {
  static uint8_t lastHue = 0;
  
  if(lastHue != hue || forceRender) {
    fill_solid(leds, LED_NUM, CRGB().setHue(hue));
    lastHue = hue;
    return true;
  }

  return false;
}