# MagicLamp

Highly refactored version of [https://kit.alexgyver.ru/tutorials/magic-lamp](https://kit.alexgyver.ru/tutorials/magic-lamp/) ([video](https://www.youtube.com/watch?v=5ySiujKswwE&t=139s) of AlexGyver's version). Supports only addressable LED strip.

Schemes are available at AlexGyver's version. Basically you need to connect rangefinder, LED strip and LED (optionally) and put all that in lamp.

# Features

## Three modes

- Hue color
- Black body radiation color
- Perlin noise fire (using hue color to color the fire)

All modes are independent in terms of brightness and hue/kelvin temperature (i.e. their parameters).

## "Magic" gestural control

The ultrasonic rangefinder is used as button. When you hold something in front of it (up to 1 meter depending on size), it counts as button press.

- One click - enable/disable
- Two clicks - change mode

Also it can be used to control parameters of effects, bringing your hand lower or higher

- Hold - change brightness
- Click and hold - change hue or kelvin temperature (depending on effect)

## Visual feedback

This firmware supports connecting a LED to PULSE_PIN (13 by default), which turns on if rangefinder "sees" something. Convenience boost is huge, as rangefinder is not always reliable and you don't always know if first click was registered, so you can adapt and slide again if you miss.

# Changes to AlexGyver's version

- LED strip usage is asynchronous
- Removed GRGB, dropped support of RGB LED strips
- Added LED connected to PULSE_PIN, which is on when there's any signal from rangefinder
- Added crossfade between effects using color blending (in addition to already existed smooth brightness change between effects)

# Settings

Files fireEffect.ino, gesturesEffect.ino and brightnessTask.ino have settings in start of file.

There's other settings within some files, but changing those unknowingly isn't recommended.

# Libraries

- [VirtualButton](https://github.com/GyverLibs/VirtualButton)
- [FastLED](https://github.com/FastLED/FastLED)
- [EEManager](https://github.com/GyverLibs/EEManager)
