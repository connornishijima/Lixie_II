/*
   ____ ___  ____ _  _     ____ ____ ____ _ ____ _        _  _ ____ _  _ _ ___ ____ ____   /
   |  | |__] |___ |\ |     [__  |___ |__/ | |__| |        |\/| |  | |\ | |  |  |  | |__/  / 
   |__| |    |___ | \|     ___] |___ |  \ | |  | |___     |  | |__| | \| |  |  |__| |  \ .  

  (115200 Baud)
*/                                                                                                                

void title(){
  Serial.println(F("----------------------------------"));
  Serial.println(F(" Lixie II Introduction Tour       "));
  Serial.println(F(" by Connor Nishijima              "));
  Serial.println(F(" November 1st, 2019               "));
  Serial.println();
  Serial.println(F(" Released under the GPLv3 License "));
  Serial.println(F("----------------------------------"));
}

#include "Lixie_II.h" // Include Lixie Library

#define DATA_PIN   13 // Pin to drive Lixies (D7 on Wemos)
#define NUM_LIXIES 4  // How many Lixies you have
Lixie_II lix(DATA_PIN, NUM_LIXIES);

uint16_t function_wait = 8000;

void setup() {
  Serial.begin(115200);
  lix.begin(); // Initialize LEDs
  lix.white_balance(Tungsten100W); // Default
    // Can be: Tungsten40W, Tungsten100W,  
    //         Halogen,     CarbonArc,
    //         HighNoonSun, DirectSunlight,
    //         OvercastSky, ClearBlueSky  
    // 2,600K - 20,000K
	delay(1000);
}


void loop() {
  title();
  run_demo();
}

void run_demo(){
  lix.color_all(ON, CRGB(0,255,255)); // Start with cyan color
  delay(1000);
  
  Serial.println("This is a tour of the basic and advanced features of the Lixie II library!\n");
  delay(2000);
  
  Serial.println("Let's begin!\n");
  delay(3000);
  
  Serial.println("Lixie II can take integers, char arrays, or floats as input with lix.write() and lix.write_float(). (Any non-numeric chars are ignored)\n");
  lix.write_float(2.0, 1);
  delay(function_wait);
  
  Serial.println("For the Nixie fans, Lixie II offers the lix.nixie() function to shortcut you to pleasant Nixie tube colors!\n");
  lix.nixie();
  lix.write(222222);
  delay(function_wait);
  
  Serial.println("Lixie II operates with 'ON' and 'OFF' colors.");
  Serial.println("Currently, the ON color is orange, and the OFF");
  Serial.println("color is a dark blue.\n");
  delay(function_wait);
  
  Serial.println("They can be configured independently in the lix.color_all(<ON/OFF>, CRGB col) function.");
  Serial.println("I've now used color_all(OFF, CRGB(0,8,0)); to change the blue to a dark green!\n");
  lix.color_all(OFF, CRGB(0,10,0));
  delay(function_wait);
  
  Serial.println("Lixies can also have two colors per digit by using lix.color_all_dual(ON, CRGB col1, CRGB col2);!\n");
  lix.color_all_dual(ON, CRGB(255,0,0), CRGB(0,0,255));
  delay(function_wait);
  
  Serial.println("Individual displays can also be colored with lix.color_display(index, <ON/OFF>, CRGB col)!\n");
  lix.color_display(1, ON, CRGB(255,0,0));
  lix.color_display(1, OFF, CRGB(0,0,10));
  delay(function_wait);
  
  Serial.println("Gradients are possible with lix.gradient_rgb(<ON/OFF>, CRGB col1, CRGB col2)!\n");
  lix.color_all(OFF, CRGB(0,0,0)); // Remove OFF color
  lix.gradient_rgb(ON, CRGB(255,0,255), CRGB(0,255,255));
  delay(function_wait);
  
  Serial.println("Lixies can also show 'streaks' at any position between left (0.0) and");
  Serial.println("right (1.0) with lix.streak(CRGB col, float pos, uint8_t blur);\n");
  lix.stop_animation(); // Necessary to prevent rendering of current numbers
  float iter = 0;
  uint32_t t_start = millis();
  while(millis() < t_start+function_wait){
    float pos = (sin(iter) + 1)/2.0;
    lix.streak(CRGB(0,255,0), pos, 8);
    iter += 0.02;
    delay(1);
  }

  Serial.println("Here's lix.streak() with 'blur' of '1' (none)\n");
  t_start = millis();
  while(millis() < t_start+function_wait){
    float pos = (sin(iter) + 1)/2.0;
    lix.streak(CRGB(0,255,0), pos, 1);
    iter += 0.02;
    delay(1);
  }

  Serial.println("These can be used to show a progress percentage, or 'busy' indicator while WiFi connects.\n");
  delay(3000);
  Serial.println("One shortcut to this functionality is lix.sweep_color(CRGB col, uint16_t speed, uint8_t blur);\n");
  for(uint8_t i = 0; i < 5; i++){
    lix.sweep_color(CRGB(0,0,255), 20, 5);
  }
  lix.start_animation(); // This resumes "number mode" after we stopped it above
  
  lix.nixie();
  Serial.println("Transitions between numbers can also be modified.");
  Serial.println("By default Lixie II uses a 250ms crossfade, but this can");
  Serial.println("be changed with lix.transition_time(ms) or lix.transition_type(<INSTANT/CROSSFADE>);\n");
  
  Serial.println("CROSSFADE...");
  for(uint8_t i = 0; i < 30; i++){
    lix.write(i);
    delay(250);
  }
  Serial.println("INSTANT!\n");
  lix.transition_type(INSTANT);
  for(uint8_t i = 0; i < 30; i++){
    lix.write(i);
    delay(250);
  }

  Serial.println("And to end our guided tour, here's a floating point of seconds elapsing, using lix.rainbow(uint8_t hue, uint8_t separation);");
  delay(3000);
  
  t_start = millis();
  float hue = 0;
  while(millis() < t_start+10000){
    uint32_t millis_passed = millis()-t_start;
    float seconds = millis_passed / 1000.0;
    lix.write_float(seconds, 2);
    lix.rainbow(hue, 20);
    hue += 0.1;
    delay(1);
  }
  lix.write_float(10.0, 2);
  delay(function_wait);
  
  Serial.println("\n\n");
}
