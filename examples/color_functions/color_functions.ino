#include <Lixie_II.h>           // https://github.com/connornishijima/Lixie_II
#define DATA_PIN        13      // Lixie DIN connects to this pin (D7 on Wemos)
#define NUM_DIGITS      4
Lixie_II lix(DATA_PIN, NUM_DIGITS);

void setup() {
	lix.begin();
}

void loop() {
  lix.write(222222); // Any number works
  
  // Sets all currently used LEDs (ON) to cyan
  lix.color_all(ON, CRGB(0,255,255));
  delay(3000);
 
  // Sets all "unused" LEDs (OFF) to a dim red
  lix.color_all(OFF, CRGB(32,0,0));
  delay(3000);

  // These "OFF" LEDs can be used creatively, such as mimicking LED underlights of a Nixie Tube:
  lix.color_all(ON, CRGB(255, 70, 7));
  lix.color_all(OFF, CRGB(0, 3, 8));  
  delay(3000);
  // (However, save yourself the time and just use lix.nixie(); as a shortcut for this!)

  // Conversely, all LEDs can be colored the same color for things such as notification effects.
  lix.color_all(ON, CRGB(255, 0, 0));
  lix.color_all(OFF, CRGB(255, 0, 0));  
  delay(3000);

  // Two colors can also be used in a single digit
  lix.color_all(OFF, CRGB(0,0,0)); // This clears the "OFF" color from before
  lix.color_all_dual(ON, CRGB(255,0,0), CRGB(0,0,255));
  delay(3000);

  // You can also color individual displays ("1" is second from the right, your first Lixie in the chain is "0"
  lix.color_display(1, ON, CRGB(0,255,0));
  delay(3000);

  // Gradients between a left and righthand color are possible as well
  lix.gradient_rgb(ON, CRGB(255,255,0), CRGB(0,255,255));
  delay(3000);
  
  // Gradients between a left and righthand color are possible as well
  lix.gradient_rgb(ON, CRGB(255,255,0), CRGB(0,255,255));
  delay(3000);
}