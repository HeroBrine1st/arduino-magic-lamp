bool kelvinEffect(uint16_t kelvin, bool forceRender) {
  static uint16_t lastKelvin = 0;
  
  if(lastKelvin != kelvin || forceRender) {
    fill_solid(leds, LED_NUM, blackBodyRadiationKelvin(kelvin));
    lastKelvin = kelvin;
    return true;
  }

  return false;
}


// credits to AlexGyver for this function
// https://github.com/GyverLibs/GRGB
/*
MIT License

Copyright (c) 2021 AlexGyver

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
// цветовая температура 1000-40000К
CRGB blackBodyRadiationKelvin(uint16_t kelvin) {
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