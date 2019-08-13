/*
 * Edgelit "animation" example
 * Counts from 0 to 4,294,967,295 with transitions and theming
 */

#include "Edgelit.h"

#define DATA_PIN 5            // Pin "D1" using Wemos D1 mini's stupid labels
#define DISPLAY_COUNT 6       // Number of displays in chain
#define DISPLAY_TYPE LIXIE_1  // Type of display (LIXIE_1, NIXIE_PIPE, etc.)
Edgelit lit(DATA_PIN, DISPLAY_COUNT, DISPLAY_TYPE);

uint32_t count = 123456;
uint8_t hue = 0;
uint8_t pos = 0;
uint8_t animation_type = 0;

void setup(){
  lit.begin();                        // Initialize the interrupts for animation
  lit.transition_type(FADE_TO_BLACK); // Can be INSTANT, CROSSFADE, or FADE_TO_BLACK
  lit.transition_time(250);           // Transition time in milliseconds. (Does nothing for INSTANT transitions)
  lit.animation_callback(animator);    // User function called at 100FPS when displays are updated
  lit.empty_displays(true);           // Allows showing background color on displays not currently showing a numeral
}

void loop() {
  lit.write(count);
  count++;
  delay(500);

  if(millis() >= 20000 && animation_type == 0){
    animation_type = 1;
  }
}

// All of this happens in the background while loop() is running your own code!
void animator(){
  hue++;
  
  // Rainbow with twinkling colors
  if(animation_type == 0){    
    if(random(1,10) == 1){ // 10% chance
      uint8_t sparkle_pos = random(0,lit.led_count()); // pick a random location in the string
      lit.color_pixel(sparkle_pos,CHSV(random(0,256),255,96), BACK); // set this random location to a random color
    }
    
    for(uint16_t i = 0; i < lit.led_count(); i++){
      CRGB col = lit.color_pixel(i, BACK); // get pixel color
      // Fade all "BACK" pixels to black over time
      col.r = col.r*0.95;
      col.g = col.g*0.95;
      col.b = col.b*0.95;
      lit.color_pixel(i, col, BACK); // set pixel color
      
      lit.color_pixel(i,CHSV(hue+i,255,255), FRONT); // Creates cycling rainbow for foreground colors
    }
  }

  // White with color scanning in background
  else if(animation_type == 1){
    pos++; // "pos" is where in the chain we're currently turning pixels on
    if(pos >= lit.led_count()){ // If we reach the end of the chain, start over
      pos = 0;
    }

    lit.color(CRGB(255,255,255),FRONT);           // Foreground color to white
    lit.color_pixel(pos, CHSV(hue,255,127),BACK); // Set one background pixel @ "pos" to "hue"

    // Fade all background pixels to black over time
    for(uint16_t i = 0; i < lit.led_count(); i++){
      CRGB col = lit.color_pixel(i, BACK); // get pixel color
      col.r = col.r*0.95;
      col.g = col.g*0.95;
      col.b = col.b*0.95;
      lit.color_pixel(i, col, BACK); // set pixel color
    }
  }
}
