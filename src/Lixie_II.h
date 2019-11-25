/*
	Lixie_II.h - Library for controlling the Lixie II!
	
	Created by Connor Nishijma July 6th, 2019
	Released under the GPLv3 License
*/

#ifndef lixie_II_h
#define lixie_II_h

#include "Arduino.h"

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
	#include <Ticker.h>	// ESP ONLY
	// FastLED has issues with the ESP8266, especially
	// when used with networking, so we fix that here.
	#define FASTLED_ESP8266_RAW_PIN_ORDER
	#define FASTLED_ALLOW_INTERRUPTS 		0
	#define FASTLED_INTERRUPT_RETRY_COUNT	0
#endif

// Aside from those issues, it's my tool of choice for WS2812B
#include "FastLED.h"

#define ON  1
#define OFF 0

#define INSTANT   		0
#define CROSSFADE 		1

// Functions
class Lixie_II
{
	public:
		Lixie_II(const uint8_t pin, uint8_t n_digits);
		void build_controller(const uint8_t pin);
		void begin();
		void transition_type(uint8_t type);
		void transition_time(uint16_t ms);
		void max_power(uint8_t V, uint16_t mA);
		void color_all(uint8_t layer, CRGB col);
		void color_all_dual(uint8_t layer, CRGB col_left, CRGB col_right);
		void color_display(uint8_t display, uint8_t layer, CRGB col);
		void gradient_rgb(uint8_t layer, CRGB col_left, CRGB col_right);
		void start_animation();
		void stop_animation();
		void write(uint32_t input);
		void write(String input);
		void write_float(float input, uint8_t dec_places = 1);
		void clear_all();
		void write_digit(uint8_t digit, uint8_t num);
		void push_digit(uint8_t number);
		void clear_digit(uint8_t digit, uint8_t num);
		void special_pane(uint8_t index, bool enabled, CRGB col1 = CRGB(0,0,0), CRGB col2 = CRGB(0,0,0));
		void mask_update();
		void fade_in();
		void fade_out();
		void brightness(float level);
	        void brightness(double level);
		void run();
		void wait();
		void streak(CRGB col, float pos, uint8_t blur);
		void sweep_color(CRGB col, uint16_t speed, uint8_t blur, bool reverse = false);
		void sweep_gradient(CRGB col_left, CRGB col_right, uint16_t speed, uint8_t blur, bool reverse = false);
		void nixie();
		void white_balance(CRGB c_adj);
		void rainbow(uint8_t r_hue, uint8_t r_sep);
		
		// ----------------------------------------------
		// Deprecated Lixie 1 functions and overloads:
		// ----------------------------------------------
		
		void brightness(uint8_t b);
		void clear(bool show_change = true);
		void clear_digit(uint8_t index, bool show_change = true);
		void show();
		void write_flip(uint32_t input, uint16_t flip_time = 100, uint8_t flip_speed = 10);
		void write_fade(uint32_t input, uint16_t fade_time = 250);
		void sweep(CRGB col, uint8_t speed = 15);
		void progress(float percent, CRGB col1, CRGB col2);
		void fill_fade_in(CRGB col, uint8_t fade_speed = 20);
		void fill_fade_out(CRGB col, uint8_t fade_speed = 20);
		void color(uint8_t r, uint8_t g, uint8_t b);
		void color(CRGB c);
		void color(uint8_t r, uint8_t g, uint8_t b, uint8_t index);
		void color(CRGB c, uint8_t index);
		void color_off(uint8_t r, uint8_t g, uint8_t b);
		void color_off(CRGB c);
		void color_off(uint8_t r, uint8_t g, uint8_t b, uint8_t index);
		void color_off(CRGB c, uint8_t index);
		void color_fade(CRGB col, uint16_t duration);
		void color_fade(CRGB col, uint16_t duration, uint8_t index);
		void color_array_fade(CRGB *cols, uint16_t duration);
		void color_array_fade(CHSV *cols, uint16_t duration);
		void color_wipe(CRGB col1, CRGB col2);
		void nixie_mode(bool enabled, bool has_aura = true);
		void nixie_aura_intensity(uint8_t val);
		
	private:
		uint8_t get_size(uint32_t input);
};

#endif
