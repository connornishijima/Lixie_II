![Lixie II](https://i.imgur.com/G5l9iJk.png)

# Lixie II for Arduino

This library is still maturing, but the functionality from the original Lixie 1/Edgelit libraries is here!
For now, you can refer to the new [Getting Started Guide](https://github.com/connornishijima/Lixie_II/blob/master/getting_started.md), the "Introduction Tour" example, or [the original library documentation](https://github.com/connornishijima/Lixie-arduino) for its usage is almost identical. (There is now support for all but a few deprecated "Lixie 1" functions. Just change any ocurrance of "Lixie" in your code with "Lixie_II".)

# WARNING

## UNLIKE THE ORIGINAL LIXIE LIBRARY, THIS ONE *REALLY* PREFERS THE ESPRESSIF/XTENSA ARCHITECTURE. (ESP8266 / ESP32)

This design choice was made to allow for non-blocking animation code, as the Ticker library or ESP32's second core are great for this. The vast majority of Lixie 1 users were using Espressif controllers anyways for a cheap gateway for the Lixies to fetch internet time, stocks, analytics, etc. This means that any Arduino Unos, Micros, Pro Minis, Megas - those old controllers are still supported, but the animation system may bog things down slightly. If you haven't tried them yet, the Espressif microcontrollers are amazing and unless you're going for low-power, (Lixies are not) AVR isn't quite right for something like Lixie II.

----------
# Installation

***The Lixie II library relies on [the FastLED library from Daniel Garcia](https://github.com/FastLED/FastLED), so make sure you have that installed as well!***

~**With Arduino Library Manager:**~ COMING SOON!

~1. Open *Sketch > Include Library > Manage Libraries* in the Arduino IDE.~
~2. Search for "Lixie_II", and select the latest version.~
~3. Click the Install button and Arduino will prepare the library and examples for you!~

**Manual Install:**

1. Click "Clone or Download" above to get an "Lixie_II-master.zip" file.
2. Extract its contents to the libraries folder in your Arduino sketchbook. ("C:/Users/**YOUR_USERNAME**/Documents/Arduino/libraries" on Windows)
3. Rename the folder from "Lixie_II-master" to "Lixie_II".
