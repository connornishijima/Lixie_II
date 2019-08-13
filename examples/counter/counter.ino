/*
 * Edgelit "counter" example
 * Counts from 0 to 4,294,967,295 with transitions and theming
 */

#include "Edgelit.h"

#define DATA_PIN 5            // Pin "D1" using Wemos D1 mini's stupid labels
#define DISPLAY_COUNT 6       // Number of displays in chain
#define DISPLAY_TYPE LIXIE_1  // Type of display (LIXIE_1, NIXIE_PIPE, etc.)
Edgelit lit(DATA_PIN, DISPLAY_COUNT, DISPLAY_TYPE);

uint32_t count = 0;

void setup(){
  lit.begin();                        // Initialize the interrupts for animation
  lit.transition_type(FADE_TO_BLACK); // Can be INSTANT, CROSSFADE, or FADE_TO_BLACK
  lit.transition_time(250);           // Transition time in milliseconds. (Does nothing for INSTANT transitions)
  lit.nixie(1);                       // Sets the color scheme to match the ionized neon/argon mix in an average Nixie tube.
                                      // The 1 is "Argon intensity", the subtle blue hue some Nixie tubes have. (0-255)
// Some other display presets to try:
// lit.vfd_green();
// lit.vfd_blue();

// Or a theme of your own:
// lit.color(CRGB(255,255,255), FRONT); // "FRONT" is coloring for numerals shown
// lit.color(CRGB(0,8,2), BACK);        // "BACK" is coloring for numerals not shown. (Background)
}

void loop() {
  lit.write(count);
  count++;
  delay(500);
}
