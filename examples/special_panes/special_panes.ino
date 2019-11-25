/*
 * Lixie II "Special Panes" Example ////////////////////////////////////////////////////
 * 
 * Lixie's 11th pane (which is a decimal point by default) can now be arbitrarily
 * controlled explicitly using the lix.special_pane() function and implicitly through
 * passing special Strings into lix.write();
 * 
 * ----------------------------------------------------------------------------
 * 
 * EXPLICIT CONTROL:
 * lix.special_pane(uint8_t index, bool enabled, [CRGB col1], [CRGB col2]);
 * 
 * This function provides explicit control of the 11th panes, and the CRGB colors
 * passed to this function override any global color settings the displays currently
 * have. To show the rightmost digit's 11th pane in pure green:
 * 
 * lix.special_pane(0, true, CRGB(0,255,0));
 * 
 * You can optionally pass a second color to be used in a fashion similar
 * to lix.color_dual():
 * 
 * lix.special_pane(0, true, CRGB(0,255,0), CRGB(0,0,255); // Green on left side, Blue on right
 * 
 * These settings will persist indefinitely, until you disable that same pane:
 * 
 * lix.special_pane(0, false);
 * 
 * ----------------------------------------------------------------------------
 * IMPLICIT CONTROL:
 * lix.write(String input);
 * 
 * When a String is passed to the lix.write(); function it will be parsed on a
 * character-by-character basis, where numerals behave as normal, spaces create
 * blank displays, and any other character is treated as a special pane character.
 * 
 * For example, if you called lix.write("2.3 ");, the '.' is treated as an 11th pane
 * character, and the trailing space leaves your rightmost Lixie blank.
 * 
 * This 11th pane behavior differs from explicit control in that it respects the
 * animation ISR and ON/OFF color groups that are already defined. This means that
 * they have normal transition-time behaviors and the 11th-pane chars will be colored
 * just as a normal numeral would at that position.
 * 
 * To disable implicity controlled 11th panes once they are on, just write a different
 * value to the displays as you normally would.
 */

#include <Lixie_II.h>           // https://github.com/connornishijima/Lixie_II
#define DATA_PIN        12      // Lixie DIN connects to this pin (D6 on Wemos)
#define NUM_DIGITS      4
Lixie_II lix(DATA_PIN, NUM_DIGITS);

void setup() {
  lix.begin(); // Mandatory, sets up animation timer
  lix.gradient_rgb(ON, CRGB(255, 0, 0), CRGB(0, 255, 0)); // Gradient from red to green
}

void loop() {
  double_panes();     // EXPLICIT
  fake_load();        // IMPLICIT
  time_with_pm_dot(); // EXPLICIT
  delay(1000);
}

void double_panes() { // double_panes() uses special panes explicitly through lix.special_pane(uint8_t index, bool enabled, CRGB col1, CRGB col2);
                      // and is independent of any lix.write() or lix.color_* animations
  
  lix.write(111111); // Writing a normal integer
  delay(1000);

  lix.write("222222"); // Passing in a string works as well
  delay(1000);

  lix.write("2222 2"); // spaces are treated as blank displays
  delay(1000);

  for (uint8_t i = 0; i < NUM_DIGITS; i++) { // enable special panes in green, one by one
    lix.special_pane(i, true, CRGB(0, 255, 0));
    delay(1000);
  }

  for (uint8_t i = 0; i < NUM_DIGITS; i++) { // enable dual-color special panes, one by one
    lix.special_pane(i, true, CRGB(0, 255, 255), CRGB(255, 0, 0));
    delay(1000);
  }

  for (uint8_t i = 0; i < NUM_DIGITS; i++) { // disable special panes, one by one
    lix.special_pane(i, false);
    delay(1000);
  }
}

void fake_load() { // fake_load() uses special panes implicitly through lix.write(String input); using spaces and periods, and respects the ON and OFF color groups
                   // An example of implicitly enabling the special pane at index 2 (rightmost display is index 0) you could run: lix.write("3.14");
  
  for (uint8_t i = 0; i < 10; i++) { // repeats 10 times
    String message;
    for (uint8_t i = 0; i < NUM_DIGITS; i++) {
      message.concat(' ');
    }

    // Creates a "scanning" animation for any display size
    for (uint8_t x = 0; x < NUM_DIGITS; x++) {
      for (uint8_t y = 0; y < NUM_DIGITS; y++) {
        if (x == y) {
          message.setCharAt(y, '.');
        }
        else {
          message.setCharAt(y, ' ');
        }
      }

      // prints: ".   ", then
      //         " .  ", then
      //         "  . ", then
      //         "   .", then loops

      lix.write(message);
      delay(250);
    }
  }
}

void time_with_pm_dot(){
  lix.write("1234"); // WRITE "TIME"

  // Blink "PM" dot on and off, in cyan, ten times
  for(uint8_t i = 0; i < 10; i++){
    lix.special_pane(3, true, CRGB(0, 255, 255));
    delay(1000);
    lix.special_pane(3, false);
    delay(1000);
  }
}
