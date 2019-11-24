/*
  Lixie_II.cpp - Library for controlling the Lixie 2!
  
  Created by Connor Nishijma July 6th, 2019
  Released under the GPLv3 License
*/

#include "Lixie_II.h"

uint8_t get_size(uint32_t input);

// FastLED info for the LEDs
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB

const uint8_t leds_per_digit = 22;
uint8_t n_digits;      // Keeps the number of displays
uint16_t n_LEDs;       // Keeps the number of LEDs based on display quantity.
CLEDController *lix_controller; // FastLED 
CRGB *lix_leds;

const uint8_t led_assignments[leds_per_digit] = { 1, 9, 4, 6, 255, 7, 3, 0, 2, 8, 5, 5, 8, 2, 0, 3, 7, 255, 6, 4, 9, 1  }; // 255 is extra pane
const uint8_t x_offsets[leds_per_digit]     = { 0, 0, 0, 0, 1,   1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 4,   5, 5, 5, 5  };
uint8_t max_x_pos = 0;

CRGB *col_on;
CRGB *col_off;

uint8_t *led_mask_0;
uint8_t *led_mask_1;

uint8_t current_mask = 0;
float mask_fader = 0.0;
float mask_push = 1.0;
bool mask_fade_finished = false;

uint8_t trans_type = CROSSFADE;
uint16_t trans_time = 250;
bool transition_mid_point = true;

float bright = 1.0;

bool background_updates = true;

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
  Ticker lixie_animation;
#endif

uint16_t led_to_x_pos(uint16_t led){
  uint8_t led_digit_pos = x_offsets[led%22];
  
  uint8_t complete_digits = 0;
  while(led >= leds_per_digit){
    led -= leds_per_digit;
    complete_digits += 1;
  }
  
  return max_x_pos - (led_digit_pos + (complete_digits*6));
}

void animate(){
  if(mask_fader < 1.0){
    mask_fader += mask_push;
  }
  
  if(mask_fader >= 1.0){
    mask_fader = 1.0;
    if(!mask_fade_finished){
      mask_fade_finished = true;
      
    }
  }
  else if(mask_fader >= 0.5){
    transition_mid_point = true;
  }
  
  for(uint16_t i = 0; i < n_LEDs; i++){
    float mask_float;
    CRGB new_col;
    uint8_t mask_input_0 = led_mask_0[i];
    uint8_t mask_input_1 = led_mask_1[i];

    if(trans_type == INSTANT || trans_type == CROSSFADE){
      if(current_mask == 0){  
        mask_float = ((mask_input_0*(1-mask_fader)) + (mask_input_1*(mask_fader)))/255.0;
      }
      else if(current_mask == 1){
        mask_float = ((mask_input_1*(1-mask_fader)) + (mask_input_0*(mask_fader)))/255.0;
      }
    }
    
    new_col.r = ((col_on[i].r*mask_float) + (col_off[i].r*(1-mask_float)))*bright;
    new_col.g = ((col_on[i].g*mask_float) + (col_off[i].g*(1-mask_float)))*bright;
    new_col.b = ((col_on[i].b*mask_float) + (col_off[i].b*(1-mask_float)))*bright;
    
    lix_leds[i] = new_col;  
    
    //Serial.print(led_to_x_pos(i));
    //Serial.print('\t');
    //Serial.println(max_x_pos);
  }
      
  lix_controller->showLeds(); 
}

void Lixie_II::transition_type(uint8_t type){
  trans_type = type;
}

void Lixie_II::transition_time(uint16_t ms){
  trans_time = ms;
}

void Lixie_II::run(){
  animate();
}

void Lixie_II::wait(){
  while(mask_fader < 1.0){
    animate();
  }
}

void Lixie_II::start_animation(){
#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
  lixie_animation.attach_ms(20, animate);
#elif defined(__AVR__)  
  // TIMER 1 for interrupt frequency 50 Hz:
  cli(); // stop interrupts
  TCCR1A = 0; // set entire TCCR1A register to 0
  TCCR1B = 0; // same for TCCR1B
  TCNT1  = 0; // initialize counter value to 0
  // set compare match register for 50 Hz increments
  OCR1A = 39999; // = 16000000 / (8 * 50) - 1 (must be <65536)
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS12, CS11 and CS10 bits for 8 prescaler
  TCCR1B |= (0 << CS12) | (1 << CS11) | (0 << CS10);
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);
  sei(); // allow interrupts
#endif
}

#if defined(__AVR__)  
ISR(TIMER1_COMPA_vect){
   animate();
}
#endif

void Lixie_II::stop_animation(){
#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
  lixie_animation.detach();
#endif
}

Lixie_II::Lixie_II(const uint8_t pin, uint8_t number_of_digits){
  n_LEDs = number_of_digits * leds_per_digit;
  n_digits = number_of_digits;
  max_x_pos = (number_of_digits * 6)-1;
  
  lix_leds = new CRGB[n_LEDs];  
  led_mask_0 = new uint8_t[n_LEDs];
  led_mask_1 = new uint8_t[n_LEDs];
    
  col_on = new CRGB[n_LEDs];
  col_off = new CRGB[n_LEDs];
  
  for(uint16_t i = 0; i < n_LEDs; i++){
    led_mask_0[i] = 0;
    led_mask_1[i] = 0;
    
    col_on[i] = CRGB(255,255,255);
    col_off[i] = CRGB(0,0,0);
  }
  
  build_controller(pin);
}

void Lixie_II::build_controller(const uint8_t pin){
  //FastLED control pin has to be defined as a constant, (not just const, it's weird) this is a hacky workaround.
  // Also, this stops you from defining non existent pins with your current board architecture
  if (pin == 0)
    lix_controller = &FastLED.addLeds<LED_TYPE, 0, COLOR_ORDER>(lix_leds, n_LEDs);
  else if (pin == 2)
    lix_controller = &FastLED.addLeds<LED_TYPE, 2, COLOR_ORDER>(lix_leds, n_LEDs);
  else if (pin == 4)
    lix_controller = &FastLED.addLeds<LED_TYPE, 4, COLOR_ORDER>(lix_leds, n_LEDs);
  else if (pin == 5)
    lix_controller = &FastLED.addLeds<LED_TYPE, 5, COLOR_ORDER>(lix_leds, n_LEDs);
  else if (pin == 12)
    lix_controller = &FastLED.addLeds<LED_TYPE, 12, COLOR_ORDER>(lix_leds, n_LEDs);
  else if (pin == 13)
    lix_controller = &FastLED.addLeds<LED_TYPE, 13, COLOR_ORDER>(lix_leds, n_LEDs);
    //FastLED.addLeds<LED_TYPE, 13, COLOR_ORDER>(lix_leds, n_LEDs);
}

void Lixie_II::begin(){
  max_power(5,500); // Default for the safety of your PC USB
  start_animation();
}

void Lixie_II::max_power(uint8_t V, uint16_t mA){
  FastLED.setMaxPowerInVoltsAndMilliamps(V, mA);
}

void Lixie_II::clear_all(){
  if(current_mask == 0){
    for(uint16_t i = 0; i < n_LEDs; i++){
      led_mask_0[i] = 0;
    }
  }
  else if(current_mask == 1){
    for(uint16_t i = 0; i < n_LEDs; i++){
      led_mask_1[i] = 0;
    }
  }
}

void Lixie_II::write(uint32_t input){
  //if(get_size(input) <= n_digits){
    clear_all();
    
    uint32_t n_place = 1;
    // Powers of 10 while avoiding floating point math
    for(uint8_t i = 1; i < get_size(input); i++){
      n_place *= 10;
    }

    for(n_place; n_place > 0; n_place /= 10){
      push_digit(input / n_place);
      if(n_place > 1) input = (input % n_place);
    }
      
  //}
  
  if(current_mask == 0){
    current_mask = 1;
  }
  else if(current_mask == 1){
    current_mask = 0;
  }
  
  mask_update();
}

bool char_is_number(char input){
  if(input <= 57 && input >= 48) // if between ASCII '9' and '0'
    return true;
  else
    return false;
}

void Lixie_II::write(char* input){
  char temp[20] = "";
  byte index = 0;
  for(uint8_t i = 0; i < 20; i++){
    if(char_is_number(input[i])){
      temp[index] = input[i];
      index++;
    }
  }
  uint32_t output = atol(temp);
  write(output);
}

void Lixie_II::write_float(float input_raw, uint8_t dec_places){
  uint16_t dec_places_10s = 1;
  float input_mult = input_raw;
  
  for(uint8_t i = 0; i < dec_places; i++){
    input_mult*=10;
    dec_places_10s*=10;
  }
  
  uint16_t input = input_mult;
  
  clear_all();
    
  uint32_t n_place = 1;
  // Powers of 10 while avoiding floating point math
  for(uint8_t i = 1; i < get_size(input); i++){
    n_place *= 10;
  }

  for(n_place; n_place > 0; n_place /= 10){
    if(n_place == (dec_places_10s/10)){
      push_digit(255);
    }
    
    push_digit(input / n_place);
    
    
    
    if(n_place > 1) input = (input % n_place);
  }
  
  if(current_mask == 0){
    current_mask = 1;
  }
  else if(current_mask == 1){
    current_mask = 0;
  }
  
  mask_update();
}

void Lixie_II::push_digit(uint8_t number) {
  // If multiple displays, move all LED states forward one
  if (n_digits > 1) {
    for (uint16_t i = n_LEDs - 1; i >= leds_per_digit; i--) {
      if(current_mask == 0){
        led_mask_0[i] = led_mask_0[i - leds_per_digit];
      }
      else{
        led_mask_1[i] = led_mask_1[i - leds_per_digit];
      }
    }
  }
  
  // Clear the LED states for the first display
  for (uint16_t i = 0; i < leds_per_digit; i++) {
    if(current_mask == 0){
      led_mask_0[i] = led_mask_0[i - leds_per_digit];
    }
    else{
      led_mask_1[i] = led_mask_1[i - leds_per_digit];
    }
  }
  
  for(uint8_t i = 0; i < leds_per_digit; i++){
    if(led_assignments[i] == number){
      if(current_mask == 0){
        led_mask_0[i] = 255;
      }
      else{
        led_mask_1[i] = 255;
      }
    }
    else{
      if(current_mask == 0){
        led_mask_0[i] = 0;
      }
      else{
        led_mask_1[i] = 0;
      }
    }
  } 
}

void Lixie_II::write_digit(uint8_t digit, uint8_t num){
  uint16_t start_index = leds_per_digit*digit;
  
  if(num < 10){
    clear_digit(digit,num);
    for(uint8_t i = 0; i < leds_per_digit; i++){      
      if(led_assignments[i] == num){
        
        if(current_mask == 0){
          led_mask_1[start_index+i] = 255;
        }
        else if(current_mask == 1){
          led_mask_0[start_index+i] = 255;
        }       
      }
    }
    
    mask_update();
  }
}

void Lixie_II::clear_digit(uint8_t digit, uint8_t num){
  uint16_t start_index = leds_per_digit*digit;
  for(uint8_t i = 0; i < 22; i++){
    if(current_mask == 0){
      led_mask_1[start_index+i] = 0;//CRGB(0*0.06,100*0.06,255*0.06);   
    }
    if(current_mask == 1){
      led_mask_0[start_index+i] = 0;//CRGB(0*0.06,100*0.06,255*0.06);   
    }
  }
  
  mask_update();
}

void Lixie_II::mask_update(){
  mask_fader = 0.0;
  
  if(trans_type == INSTANT){
    mask_push = 1.0;
  }
  else{
    float trans_multiplier = trans_time / float(1000);
    mask_push = 1 / (50.0 * trans_multiplier);
  }
  mask_fade_finished = false;
  transition_mid_point = false;
  
  // WAIT GOES HERE
}

void Lixie_II::color_all(uint8_t layer, CRGB col){
  for(uint16_t i = 0; i < n_LEDs; i++){
    if(layer == ON){
      col_on[i] = col;
    }
    else if(layer == OFF){
      col_off[i] = col;
    }
  }
}

void Lixie_II::color_all_dual(uint8_t layer, CRGB col_left, CRGB col_right){
  bool side = 1;
  for(uint16_t i = 0; i < n_LEDs; i++){
    if(i % (leds_per_digit/2) == 0){
      side = !side;
    }
    
    if(layer == ON){
      if(side){
        col_on[i] = col_left;
      }
      else{
        col_on[i] = col_right;
      }
    }
    else if(layer == OFF){
      if(side){
        col_off[i] = col_left;
      }
      else{
        col_off[i] = col_right;
      }
    }
  }
}

void Lixie_II::color_display(uint8_t display, uint8_t layer, CRGB col){
  uint16_t start_index = leds_per_digit*display;
  for(uint16_t i = 0; i < leds_per_digit; i++){
    if(layer == ON){
      col_on[start_index+i] = col;
    }
    else if(layer == OFF){
      col_off[start_index+i] = col;
    }
  }
}

void Lixie_II::gradient_rgb(uint8_t layer, CRGB col_left, CRGB col_right){
  for(uint16_t i = 0; i < n_LEDs; i++){
    float progress = 1-(led_to_x_pos(i)/float(max_x_pos));

    CRGB col_out = CRGB(0,0,0);
    col_out.r = (col_right.r*(1-progress)) + (col_left.r*(progress));
    col_out.g = (col_right.g*(1-progress)) + (col_left.g*(progress));
    col_out.b = (col_right.b*(1-progress)) + (col_left.b*(progress));
    
    if(layer == ON){
      col_on[i] = col_out;
    }
    else if(layer == OFF){
      col_off[i] = col_out;
    }
  }
}

void Lixie_II::brightness(float level){
  //FastLED.setBrightness(255*level); // NOT SUPPORTED WITH CLEDCONTROLLER :(
  bright = level; // We instead enforce brightness in the animation ISR
}

void Lixie_II::brightness(double level){
  //FastLED.setBrightness(255*level); // NOT SUPPORTED WITH CLEDCONTROLLER :(
  bright = level; // We instead enforce brightness in the animation ISR
}

void Lixie_II::fade_in(){
  for(int16_t i = 0; i < 255; i++){
    brightness(i/255.0);
    FastLED.delay(1);
  }
  brightness(1.0);
}

void Lixie_II::fade_out(){
  for(int16_t i = 255; i > 0; i--){
    brightness(i/255.0);
    FastLED.delay(1);
  }
  brightness(0.0);
}

void Lixie_II::streak(CRGB col, float pos, uint8_t blur){
  float pos_whole = pos*n_digits*6; // 6 X-positions in a single display
  
  for(uint16_t i = 0; i < n_LEDs; i++){
    uint16_t pos_delta = abs(led_to_x_pos(i) - pos_whole);
    if(pos_delta > blur){
      pos_delta = blur;
    }
    float pos_level = 1-(pos_delta/float(blur));
    
    pos_level *= pos_level; // Squared for sharper falloff
    
    lix_leds[i] = CRGB(col.r * pos_level, col.g * pos_level, col.b * pos_level);
  }
  lix_controller->showLeds();
}

void Lixie_II::sweep_color(CRGB col, uint16_t speed, uint8_t blur, bool reverse){
  stop_animation();
  sweep_gradient(col, col, speed, blur, reverse);
  start_animation();
}

void Lixie_II::sweep_gradient(CRGB col_left, CRGB col_right, uint16_t speed, uint8_t blur, bool reverse){
  stop_animation();
  
  if(!reverse){
    for(int16_t sweep_pos = (blur*-1); sweep_pos <= max_x_pos+(blur); sweep_pos++){
      int16_t sweep_pos_fixed = sweep_pos;
      if(sweep_pos < 0){
        sweep_pos_fixed = 0;
      }
      if(sweep_pos > max_x_pos){
        sweep_pos_fixed = max_x_pos;
      }
      float progress = 1-(sweep_pos_fixed/float(max_x_pos));

      CRGB col_out = CRGB(0,0,0);
      col_out.r = (col_right.r*(1-progress)) + (col_left.r*(progress));
      col_out.g = (col_right.g*(1-progress)) + (col_left.g*(progress));
      col_out.b = (col_right.b*(1-progress)) + (col_left.b*(progress));
      
      streak(col_out, 1-progress, blur);
      FastLED.delay(speed);
    }
  }
  else{
    for(int16_t sweep_pos = max_x_pos+(blur); sweep_pos >= (blur*-1); sweep_pos--){
      int16_t sweep_pos_fixed = sweep_pos;
      if(sweep_pos < 0){
        sweep_pos_fixed = 0;
      }
      if(sweep_pos > max_x_pos){
        sweep_pos_fixed = max_x_pos;
      }
      float progress = 1-(sweep_pos_fixed/float(max_x_pos));

      CRGB col_out = CRGB(0,0,0);
      col_out.r = (col_right.r*(1-progress)) + (col_left.r*(progress));
      col_out.g = (col_right.g*(1-progress)) + (col_left.g*(progress));
      col_out.b = (col_right.b*(1-progress)) + (col_left.b*(progress));
      
      streak(col_out, progress, blur);
      FastLED.delay(speed);
    }
  }
  start_animation();
}

uint8_t Lixie_II::get_size(uint32_t input){
  uint8_t places = 1;
  while(input > 9){
    places++;
    input /= 10;
  }
  return places;
}

void Lixie_II::nixie(){
  color_all(ON, CRGB(255, 70, 7));
  color_all(OFF, CRGB(0, 3, 8));  
}

void Lixie_II::white_balance(CRGB c_adj){
  lix_controller->setTemperature(c_adj);
}

void Lixie_II::rainbow(uint8_t r_hue, uint8_t r_sep){
  for(uint8_t i = 0; i < n_digits; i++){
    color_display(i, ON, CHSV(r_hue,255,255));
    r_hue+=r_sep;
  }
}

void Lixie_II::clear(bool show_change){
  for(uint16_t i = 0; i < n_LEDs; i++){
    led_mask_0[i] = 0.0;
    led_mask_1[i] = 0.0;
  }
  if(show_change){
    mask_fader = 0.0;
    mask_push  = 1.0;
    mask_fade_finished = false;
  }
}

void Lixie_II::clear_digit(uint8_t index, bool show_change){
  uint16_t start_index = index*leds_per_digit;  
  for(uint16_t i = start_index; i < leds_per_digit; i++){
    led_mask_0[i] = 0.0;
    led_mask_1[i] = 0.0;
  }
}

void Lixie_II::show(){
  mask_update();
}




// BEGIN LIXIE 1 DEPRECATED FUNCTIONS




void Lixie_II::brightness(uint8_t b){
  brightness( b/255.0 ); // Forward to newer float function
}

void Lixie_II::write_flip(uint32_t input, uint16_t flip_time, uint8_t flip_speed){
  // This animation no longer supported, crossfade is used instead
  transition_type(CROSSFADE);
  transition_time(flip_time);
  write(input);
}

void Lixie_II::write_fade(uint32_t input, uint16_t fade_time){
  transition_type(CROSSFADE);
  transition_time(fade_time);
  write(input);
}

void Lixie_II::sweep(CRGB col, uint8_t speed){
  sweep_color(col, speed, 3, false);
}

void Lixie_II::progress(float percent, CRGB col1, CRGB col2){
  uint16_t crossover_whole = percent * n_digits;
  for(uint8_t i = 0; i < n_digits; i++){
    if(n_digits-i-1 > crossover_whole){
      color_display(n_digits-i-1, ON, col1);
      color_display(n_digits-i-1, OFF, col1);
    }
    else{
      color_display(n_digits-i-1, ON, col2);
      color_display(n_digits-i-1, OFF, col2);
    }
  }
}

void Lixie_II::fill_fade_in(CRGB col, uint8_t fade_speed){
  for(float fade = 0.0; fade < 1.0; fade += 0.05){
    for(uint16_t i = 0; i < n_LEDs; i++){
      lix_leds[i].r = col.r*fade;
      lix_leds[i].g = col.g*fade;
      lix_leds[i].b = col.b*fade;
    }
    
    FastLED.show();
    delay(fade_speed);
  }
}

void Lixie_II::fill_fade_out(CRGB col, uint8_t fade_speed){
  for(float fade = 1; fade > 0; fade -= 0.05){
    for(uint16_t i = 0; i < n_LEDs; i++){
      lix_leds[i].r = col.r*fade;
      lix_leds[i].g = col.g*fade;
      lix_leds[i].b = col.b*fade;
    }
    
    FastLED.show();
    delay(fade_speed);
  }
}

void Lixie_II::color(uint8_t r, uint8_t g, uint8_t b){
  color_all(ON,CRGB(r,g,b));
}
void Lixie_II::color(CRGB c){
  color_all(ON,c);
}
void Lixie_II::color(uint8_t r, uint8_t g, uint8_t b, uint8_t index){
  color_display(index, ON, CRGB(r,g,b));
}
void Lixie_II::color(CRGB c, uint8_t index){
  color_display(index, ON, c);
}
void Lixie_II::color_off(uint8_t r, uint8_t g, uint8_t b){
  color_all(OFF,CRGB(r,g,b));
}
void Lixie_II::color_off(CRGB c){
  color_all(OFF,c);
}
void Lixie_II::color_off(uint8_t r, uint8_t g, uint8_t b, uint8_t index){
  color_display(index, OFF, CRGB(r,g,b));
}
void Lixie_II::color_off(CRGB c, uint8_t index){
  color_display(index, OFF, c);
}

void Lixie_II::color_fade(CRGB col, uint16_t duration){
  // not supported
  color_all(ON,col);
}
void Lixie_II::color_fade(CRGB col, uint16_t duration, uint8_t index){
  // not supported
  color_display(index,ON,col);
}
void Lixie_II::color_array_fade(CRGB *cols, uint16_t duration){
  // support removed
}
void Lixie_II::color_array_fade(CHSV *cols, uint16_t duration){
  // support removed
}
void Lixie_II::color_wipe(CRGB col1, CRGB col2){
  gradient_rgb(ON,col1,col2);
}
void Lixie_II::nixie_mode(bool enabled, bool has_aura){
  // enabled removed
  // has_aura removed
  nixie();
}
void Lixie_II::nixie_aura_intensity(uint8_t val){
  // support removed
}
    
