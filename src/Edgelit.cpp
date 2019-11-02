/*
	Edgelit.cpp - Library for controlling any WS2812B-based edgelit
	numeric display, such as the Lixie or NixiePipe.
	
	Created by Connor Nishijma April 17, 2018
	Released under the GPLv3 License
*/

#include "Edgelit.h"

// ##################################################################################
// CUSTOM DISPLAY TYPES CAN BE ADDED HERE! ------------------------------------------
// Just make sure to update the array lengths! (The bracketed numbers)
// You'll also need to add a #define in the Edgelit.h file that points to
// the index of your display in these arrays. For example:

// #define CUSTOM_TYPE 2

// How many e_leds are on each type of display?
const uint8_t e_led_count_pcb[2] = {
	20, // Lixie 1
	10, // NixiePipe
};

// Going through the LED strip from beginning to end, what numerals do these lights pertain to?
// -1 == IGNORE LED (if led count per board is lower than 22
// -2 == AUXILARY PANE for non-numeric use if your display has this (decimal, for example)
const int16_t e_numeral_positions[2][22] = {
	{3, 9, 2, 0, 1, 6, 5, 7, 4, 8, 3, 9, 2, 0, 1, 6, 5, 7, 4, 8,-1,-1}, // Lixie 1
	{9, 8, 7, 6, 5, 4, 3, 2, 1, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1} // NixiePipe
};

// ----------------------------------------------------------------------------------
// ##################################################################################

// Huge mess of variables starts here ----------------------------

uint8_t e_disp_type = 0;  // Stores display type (Lixie, NixiePipe, Etc.) for reference during control
uint16_t e_NumLEDs;       // Keeps the number of LEDs based on board type and display quantity.
uint8_t e_NumDigits;      // Defined by the user at compile time, set in Edgelit::Edgelit() below
uint8_t e_LEDsPerDisplay; // It's... the e_LEDsPerDisplay.

uint8_t e_trans_type = INSTANT;	// Transition type when writing a new number to displays.
								// Defaults to INSTANT (no transition)

uint16_t e_trans_time = 1;		// Default transition time (ms) is 1, to avoid possible division by zero.

CLEDController *e_controller; // FastLED 

float e_master_brightness = 1.0;

bool *e_numeral_changed;
float *e_numeral_brightness;
float *e_numeral_brightness_target;
float e_numeral_brightness_push = 1;

CRGB *e_leds;
CRGB *e_on_colors;
CRGB *e_off_colors;
uint8_t *e_mask0;
uint8_t *e_mask1;
uint8_t *e_mask_temp;
float e_mask_fader = 1.0;
float e_mask_push = 0.0;

uint8_t e_fade_step = 2; // DONE
uint8_t e_new_fade_step = 2; // DONE
uint8_t *e_current_digits;
uint8_t *e_new_digits;
bool *e_digits_used;

uint8_t e_current_mask = 0;

bool e_enable_empty_displays = false;
bool e_halt_updating = false;

os_timer_t edge_isr;

// Huge mess of variables ends here, thank god that's over -------

void nothing(){
	// Used as default animation callback
	// It does nothing, but it "does nothing" really well!
}

void (*animation_func)() = nothing; // Default can't be NULL. Sad!

void edge_run(void *parameters){
	if(!e_halt_updating){		
		if(e_mask_fader < 1 && e_trans_type != FADE_TO_BLACK){
			e_mask_fader+=e_mask_push;
		}
		
		if(e_mask_fader < 0){
			e_mask_fader = 0;
		}
		else if(e_mask_fader > 1){
			e_mask_fader = 1;
		}
		
		bool done_fading = true;
		for(uint8_t i = 0; i < e_NumDigits; i++){
			if(e_numeral_brightness[i] != e_numeral_brightness_target[i]){
				if(e_numeral_brightness[i] < e_numeral_brightness_target[i]){
					e_numeral_brightness[i] += e_numeral_brightness_push;
				}
				else if(e_numeral_brightness[i] > e_numeral_brightness_target[i]){
					e_numeral_brightness[i] -= e_numeral_brightness_push;
				}
			}
			
			if(e_numeral_brightness[i] < 0){
				e_numeral_brightness[i] = 0;
			}
			
			if(e_numeral_brightness[i] > 1){
				e_numeral_brightness[i] = 1;
			}
			
			if(e_numeral_brightness[i] != e_numeral_brightness_target[i]){
				done_fading = false;
			}
			
			if(e_current_digits[i] != e_new_digits[i]){
				e_numeral_changed[i] = true;
			}
			else{
				e_numeral_changed[i] = false;
			}

			if(e_trans_type == FADE_TO_BLACK){
				// In charge of FADE_TO_BLACK animation steps
				if(e_numeral_changed[i]){
					if(e_fade_step == 0 && done_fading){
						e_mask_fader = 0.0;
						e_numeral_brightness_target[i] = 0.0;
						e_new_fade_step = 1;
					}
					else if(e_fade_step == 1 && done_fading){
						e_mask_fader = 1.0;
						if(e_digits_used[i]){
							e_numeral_brightness_target[i] = 1.0;
						}
						e_new_fade_step = 2;
					}
					else if(e_fade_step == 2 && done_fading){
						e_current_digits[i] = e_new_digits[i];
					}
				}
			}
			else{
				e_current_digits[i] = e_new_digits[i];
			}
		}
		
		e_fade_step = e_new_fade_step;
		
		for(uint16_t i = 0; i < e_NumLEDs; i++){
			if(e_current_mask == 0){
				e_mask_temp[i] = e_mask0[i]*e_mask_fader + e_mask1[i]*(1-e_mask_fader);
			}
			else if(e_current_mask == 1){
				e_mask_temp[i] = e_mask1[i]*e_mask_fader + e_mask0[i]*(1-e_mask_fader);
			}
			
			float mask_float = (e_mask_temp[i]/255.0) * e_numeral_brightness[i/e_LEDsPerDisplay];
			
			if(e_digits_used[i/e_LEDsPerDisplay] == false){
				e_numeral_brightness_target[i/e_LEDsPerDisplay] = 0.0;
				if(e_trans_type == INSTANT){
					e_numeral_brightness[i/e_LEDsPerDisplay] = 0.0;
				}
			}
			else{
				if(e_trans_type != FADE_TO_BLACK){
					e_numeral_brightness[i/e_LEDsPerDisplay] = 1.0;
					e_numeral_brightness_target[i/e_LEDsPerDisplay] = 1.0;
				}
			}	

			// temp_brightness is used to keep "off colors" from appearing on
			// displays without a numeral currently shown on them. Using
			// empty_displays(true) will force temp_brightness to 1.
			float temp_brightness;
			if(e_enable_empty_displays){
				temp_brightness = 1.0;
			}
			else{
				temp_brightness = e_numeral_brightness[i/e_LEDsPerDisplay];
			}
			
			e_leds[i].r = (e_on_colors[i].r * mask_float + e_off_colors[i].r * (1-mask_float))*temp_brightness*e_master_brightness;
			e_leds[i].g = (e_on_colors[i].g * mask_float + e_off_colors[i].g * (1-mask_float))*temp_brightness*e_master_brightness;
			e_leds[i].b = (e_on_colors[i].b * mask_float + e_off_colors[i].b * (1-mask_float))*temp_brightness*e_master_brightness;
		}
		
		animation_func();
		e_controller->showLeds(); // Show our work!		
	}
}

Edgelit::Edgelit(const uint8_t pin, uint8_t nDigits, uint8_t d_type){
	e_disp_type = d_type;
	e_LEDsPerDisplay = e_led_count_pcb[e_disp_type];
	e_NumDigits = nDigits;
	e_NumLEDs = nDigits*e_LEDsPerDisplay;
	
	e_leds = new CRGB[e_NumLEDs];
	e_on_colors = new CRGB[e_NumLEDs];
	e_off_colors = new CRGB[e_NumLEDs];
	e_mask0 = new uint8_t[e_NumLEDs];
	e_mask1 = new uint8_t[e_NumLEDs];
	e_mask_temp = new uint8_t[e_NumLEDs];
	
	e_current_digits = new uint8_t[e_NumDigits];
	e_new_digits = new uint8_t[e_NumDigits];
	e_digits_used = new bool[e_NumDigits];
	e_numeral_changed = new bool[e_NumDigits];
	e_numeral_brightness = new float[e_NumDigits];
	e_numeral_brightness_target = new float[e_NumDigits];
	
	for(uint16_t i = 0; i < e_NumLEDs; i++){
		e_leds[i] = CRGB(0,0,0);
		e_on_colors[i] = CRGB(255,255,255);
		e_off_colors[i] = CRGB(0,0,0);
		e_mask0[i] = 0;
		e_mask1[i] = 0;
		e_mask_temp[i] = 0;
	}
	
	for(uint8_t i = 0; i < e_NumDigits; i++){
		e_numeral_changed[i] = false;
		e_numeral_brightness[i] = 1.0;
		e_numeral_brightness_target[i] = 1.0;
		
		e_digits_used[i] = false;
		e_current_digits[i] = 10; // Not a valid number to start with
		e_new_digits[i] = 10;
	}
	
	build_controller(pin);
}

void Edgelit::begin() {
	#ifdef ESP8266
		os_timer_setfn(&edge_isr, edge_run, NULL);
		os_timer_arm(&edge_isr, 10, true); // ~100 FPS

	#else
		// TIMER 1 for interrupt frequency 100 Hz:
		cli(); // stop interrupts
		TCCR1A = 0; // set entire TCCR1A register to 0
		TCCR1B = 0; // same for TCCR1B
		TCNT1  = 0; // initialize counter value to 0
		// set compare match register for 100 Hz increments
		OCR1A = F_CPU / (8 * 100) - 1; // (must be <65536)
		// turn on CTC mode
		TCCR1B |= (1 << WGM12);
		// Set CS12, CS11 and CS10 bits for 8 prescaler
		TCCR1B |= (0 << CS12) | (1 << CS11) | (0 << CS10);
		// enable timer compare interrupt
		TIMSK1 |= (1 << OCIE1A);
		sei(); // allow interrupts
	#endif
	
	max_power(5,500);
}

void Edgelit::max_power(uint8_t volts, uint16_t milliamps){
	FastLED.setMaxPowerInVoltsAndMilliamps(volts,milliamps);
}

void Edgelit::transition_type(uint8_t type){
	e_trans_type = type;
}

void Edgelit::transition_time(uint16_t ms){ // Supports down to 50ms
	e_trans_time = ms;
	e_mask_push = 1 / float(e_trans_time/10.0);
	
	e_numeral_brightness_push = e_mask_push*2; // Used for FADEOUT animation
}

uint8_t Edgelit::get_size(uint32_t input) const{
	uint8_t places = 1;
	while(input > 9){
		places++;
		input /= 10;
	}
	return places;
}

void Edgelit::clear() {
	if(e_current_mask == 0){
		memset(e_mask0, 0, e_NumLEDs);
	}
	else if(e_current_mask == 1){
		memset(e_mask1, 0, e_NumLEDs);
	}

	memset(e_new_digits, 0, e_NumDigits);
	memset(e_digits_used, 0, e_NumDigits);
	memset(e_mask_temp, 0, e_NumLEDs);
}

void Edgelit::push_digit(int16_t number) {
	if(number > 9) return;

	// If multiple displays, move all LED states forward one
	if (e_NumDigits > 1) {
		for (uint16_t i = e_NumLEDs - 1; i >= e_LEDsPerDisplay; i--) {
			if(e_current_mask == 0){
				e_mask0[i] = e_mask0[i - e_LEDsPerDisplay];
			}
			else{
				e_mask1[i] = e_mask1[i - e_LEDsPerDisplay];
			}
		}
		for(uint8_t i = e_NumDigits-1; i >= 1; i--){
			e_new_digits[i] = e_new_digits[i-1];
			e_digits_used[i] = e_digits_used[i-1];
		}
	}
	
	e_new_digits[0] = number;
	e_digits_used[0] = true;
 
	// Clear the LED states for the first display
	for (uint16_t i = 0; i < e_LEDsPerDisplay; i++) {
		if(e_current_mask == 0){
			e_mask0[i] = e_mask0[i - e_LEDsPerDisplay];
		}
		else{
			e_mask1[i] = e_mask1[i - e_LEDsPerDisplay];
		}
	}
	
	for(uint8_t i = 0; i < e_LEDsPerDisplay; i++){
		if(e_numeral_positions[e_disp_type][i] == number){
			if(e_current_mask == 0){
				e_mask0[i] = 255;
			}
			else{
				e_mask1[i] = 255;
			}
		}
		else{
			if(e_current_mask == 0){
				e_mask0[i] = 0;
			}
			else{
				e_mask1[i] = 0;
			}
		}
	}	
}

void queue_animation(){
	if(e_trans_type == INSTANT){
		e_mask_fader = 1.0;
		e_trans_time = 1;
	}
	else if(e_trans_type == CROSSFADE){
		if(e_mask_fader == 1.0){ // If previous animation is done
			e_mask_fader = 0.0;
		}
	}
	else if(e_trans_type == FADE_TO_BLACK){
		e_mask_fader = 0;
		e_fade_step = 0;
		e_new_fade_step = 0;
	}
}

void Edgelit::write(uint32_t input){
	e_current_mask = !e_current_mask; // switch between masks
	uint32_t nPlace = 1;

	clear();

	// Powers of 10 while avoiding floating point math
	for(uint8_t i = 1; i < get_size(input); i++){
		nPlace *= 10;
	}

	for(nPlace; nPlace > 0; nPlace /= 10){
		push_digit(input / nPlace);
		if(nPlace > 1) input = (input % nPlace);
	}
	
	queue_animation();
}

void Edgelit::color(CRGB col, uint8_t layer){
	for(uint16_t i = 0; i < e_NumLEDs; i++){
		if(layer == FRONT){
			e_on_colors[i] = col;
		}
		else if(layer == BACK){
			e_off_colors[i] = col;
		}
	}
}

void Edgelit::color_pixel(uint16_t led, CRGB col, uint8_t layer){
	if(layer == FRONT){
		e_on_colors[led] = col;
	}
	else if(layer == BACK){
		e_off_colors[led] = col;
	}
}

CRGB Edgelit::color_pixel(uint16_t led, uint8_t layer){
	if(layer == FRONT){
		return e_on_colors[led];
	}
	else if(layer == BACK){
		return e_off_colors[led];
	}
}

void Edgelit::color_display(uint16_t disp, CRGB col, uint8_t layer){
	for(uint8_t i = 0; i < e_LEDsPerDisplay; i++){
		if(layer == FRONT){
			e_on_colors[(e_LEDsPerDisplay*disp)+i] = col;
		}
		else if(layer == BACK){
			e_off_colors[(e_LEDsPerDisplay*disp)+i] = col;
		}
	}
}

void Edgelit::nixie(uint8_t argon_intensity){
	for(uint16_t i = 0; i < e_NumLEDs; i++){
		e_on_colors[i] = CRGB(255,70,7);    // NEON
		e_off_colors[i] = CRGB(50,150,255); // ARGON
		
		e_off_colors[i].r *= (argon_intensity/255.0);
		e_off_colors[i].g *= (argon_intensity/255.0);
		e_off_colors[i].b *= (argon_intensity/255.0);
	}
}

void Edgelit::vfd_green(uint8_t aura_intensity){
	for(uint16_t i = 0; i < e_NumLEDs; i++){
		e_on_colors[i] = CRGB(100,255,100);  // ION
		e_off_colors[i] = CRGB(100,255,100); // AURA
		
		e_off_colors[i].r *= (aura_intensity/255.0);
		e_off_colors[i].g *= (aura_intensity/255.0);
		e_off_colors[i].b *= (aura_intensity/255.0);
	}
}

void Edgelit::vfd_blue(uint8_t aura_intensity){
	for(uint16_t i = 0; i < e_NumLEDs; i++){
		e_on_colors[i] = CRGB(150,255,255);  // ION
		e_off_colors[i] = CRGB(0,100,255);   // AURA
		
		e_off_colors[i].r *= (aura_intensity/255.0);
		e_off_colors[i].g *= (aura_intensity/255.0);
		e_off_colors[i].b *= (aura_intensity/255.0);
	}
}

void Edgelit::progress(float percent, float brightness){
	memset(e_digits_used, 1, e_NumDigits);
	memset(e_numeral_brightness-1, brightness, e_NumDigits);
	memset(e_numeral_brightness_target-1, brightness, e_NumDigits);	// Not sure why e_numeral_brightness has to be offset by 1 here
	e_mask_fader = 1.0;
	e_fade_step = 0;
	
	uint16_t index = e_NumLEDs-(e_NumLEDs*(percent/100.0));
	
	for(uint16_t i = 0; i < e_NumLEDs; i++){
		if(i <= index){
			e_mask0[i] = 0;
			e_mask1[i] = 0;
		}
		else{
			e_mask0[i] = 255;
			e_mask1[i] = 255;
		}
		yield();
	}
}

void Edgelit::animation_callback(void (*func)()) {
	 animation_func = func;
}

uint16_t Edgelit::led_count(){
	return e_NumLEDs;
}

void Edgelit::empty_displays(bool state){
	e_enable_empty_displays = state;
}

void Edgelit::brightness(float bright){
	e_master_brightness = bright;
}

// Sets white_balance correction for the displays, defaults to Tungsten100W.
// http://fastled.io/docs/3.1/group___color_enums.html#gadf6bcba67c9573665af20788c4431ae8
void Edgelit::white_balance(CRGB correction){
	e_controller->setCorrection(correction);
}


void Edgelit::build_controller(const uint8_t pin){
	//FastLED control pin has to be defined as a constant, (not just const, it's weird) this is a hacky workaround.
	// Also, this stops you from defining non existent pins with your current board architecture
	if (pin == 0)
		e_controller = &FastLED.addLeds<LED_TYPE, 0, COLOR_ORDER>(e_leds, e_NumLEDs);
	else if (pin == 2)
		e_controller = &FastLED.addLeds<LED_TYPE, 2, COLOR_ORDER>(e_leds, e_NumLEDs);
	else if (pin == 4)
		e_controller = &FastLED.addLeds<LED_TYPE, 4, COLOR_ORDER>(e_leds, e_NumLEDs);
	else if (pin == 5)
		e_controller = &FastLED.addLeds<LED_TYPE, 5, COLOR_ORDER>(e_leds, e_NumLEDs);
	else if (pin == 12)
		e_controller = &FastLED.addLeds<LED_TYPE, 12, COLOR_ORDER>(e_leds, e_NumLEDs);
}
