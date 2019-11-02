#include <Lixie_II.h>           // https://github.com/connornishijima/Lixie_II
#define DATA_PIN        13      // Lixie DIN connects to this pin (D7 on Wemos)
#define NUM_DIGITS      4
Lixie_II lix(DATA_PIN, NUM_DIGITS);

void setup() {
  lix.begin();
  lix.transition_type(INSTANT); // Defaults to CROSSFADE of 250ms
  lix.gradient_rgb(ON, CRGB(0,255,0), CRGB(0,255,255)); // Gradient from green to cyan
}

void loop() {
  uint32_t time_millis = millis();
  float seconds = time_millis / 1000.0;
  lix.write_float(seconds, 2); // Writes a floating point number (of the seconds elapsed) with 2 decimal places.
  delay(1);
}