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

const uint8_t led_assignments[leds_per_digit] = {	1, 9, 4, 6, 255, 7, 3, 0, 2, 8, 5, 5, 8, 2, 0, 3, 7, 255, 6, 4, 9, 1	}; // 255 is extra pane
const uint8_t x_offsets[leds_per_digit] 	  = {	0, 0, 0, 0, 1,   1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 4,   5, 5, 5, 5	};
uint8_t max_x_pos = 0;

CRGB *color_on;
CRGB *color_off;

uint8_t *led_mask_0;
uint8_t *led_mask_1;

uint8_t current_mask = 0;
float mask_fader = 0.0;
float mask_push = 1.0;
bool mask_fade_finished = false;

uint8_t transition_type = CROSSFADE;
bool transition_mid_point = true;

Ticker lixie_animation;

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

		if(transition_type == INSTANT || transition_type == CROSSFADE){
			if(current_mask == 0){	
				mask_float = ((mask_input_0*(1-mask_fader)) + (mask_input_1*(mask_fader)))/255.0;
			}
			else if(current_mask == 1){
				mask_float = ((mask_input_1*(1-mask_fader)) + (mask_input_0*(mask_fader)))/255.0;
			}
		}
		
		new_col.r = (color_on[i].r*mask_float) + (color_off[i].r*(1-mask_float));
		new_col.g = (color_on[i].g*mask_float) + (color_off[i].g*(1-mask_float));
		new_col.b = (color_on[i].b*mask_float) + (color_off[i].b*(1-mask_float));
		
		lix_leds[i] = new_col;	
		
		//Serial.print(led_to_x_pos(i));
		//Serial.print('\t');
		//Serial.println(max_x_pos);
	}
			
	lix_controller->showLeds();	
}

void Lixie_II::start_animation(){
	lixie_animation.attach_ms(20, animate);
}

void Lixie_II::stop_animation(){
	lixie_animation.detach();
}

Lixie_II::Lixie_II(const uint8_t pin, uint8_t number_of_digits){
	n_LEDs = number_of_digits * leds_per_digit;
	n_digits = number_of_digits;
	max_x_pos = (number_of_digits * 6)-1;
	
	lix_leds = new CRGB[n_LEDs];	
	led_mask_0 = new uint8_t[n_LEDs];
	led_mask_1 = new uint8_t[n_LEDs];
		
	color_on = new CRGB[n_LEDs];
	color_off = new CRGB[n_LEDs];
	
	for(uint16_t i = 0; i < n_LEDs; i++){
		led_mask_0[i] = 0;
		led_mask_1[i] = 0;
		
		color_on[i] = CRGB(255,255,255);
		color_off[i] = CRGB(0,0,0);
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
	
	if(transition_type == INSTANT){
		mask_push = 1.0;
	}
	else{
		mask_push = 0.1;
	}
	mask_fade_finished = false;
	transition_mid_point = false;
}

void Lixie_II::color_all(uint8_t layer, CRGB col){
	for(uint16_t i = 0; i < n_LEDs; i++){
		if(layer == ON){
			color_on[i] = col;
		}
		else if(layer == OFF){
			color_off[i] = col;
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
				color_on[i] = col_left;
			}
			else{
				color_on[i] = col_right;
			}
		}
		else if(layer == OFF){
			if(side){
				color_off[i] = col_left;
			}
			else{
				color_off[i] = col_right;
			}
		}
	}
}

void Lixie_II::color_display(uint8_t display, uint8_t layer, CRGB col){
	uint16_t start_index = leds_per_digit*display;
	for(uint16_t i = 0; i < leds_per_digit; i++){
		if(layer == ON){
			color_on[start_index+i] = col;
		}
		else if(layer == OFF){
			color_off[start_index+i] = col;
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
			color_on[i] = col_out;
		}
		else if(layer == OFF){
			color_off[i] = col_out;
		}
	}
}

void Lixie_II::streak(CRGB col, int16_t pos, uint8_t blur){
	for(uint16_t i = 0; i < n_LEDs; i++){
		
		uint16_t pos_delta = abs(led_to_x_pos(i) - pos);
		if(pos_delta > blur){
			pos_delta = blur;
		}
		float pos_level = 1-(pos_delta/float(blur));
		
		pos_level*=pos_level;
		
		if(i == 0){
			//Serial.println(pos_level);
		}
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
			
			streak(col_out, sweep_pos, blur);
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
			
			streak(col_out, sweep_pos, blur);
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

/*
void fill_all(CRGB col){
  for(uint16_t i = 0; i < n_LEDs; i++){
    led_mask[i] = 255;
	color_on[i] = col;
  }
}
*/