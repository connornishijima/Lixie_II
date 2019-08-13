/*
	Lixie_II.h - Library for controlling the Lixie II!
	
	Created by Connor Nishijma July 6th, 2019
	Released under the GPLv3 License
*/

#ifndef lixie_II_h
#define lixie_II_h

#include "Arduino.h"
#include <Ticker.h>	// ESP ONLY

// FastLED has issues with the ESP8266, especially
// when used with networking, so we fix that here.
#define FASTLED_ESP8266_RAW_PIN_ORDER
#define FASTLED_ALLOW_INTERRUPTS 		0
#define FASTLED_INTERRUPT_RETRY_COUNT	0

// Aside from those issues, it's my tool of choice for WS2812B
#include "FastLED.h"

#define ON  1
#define OFF 0

#define INSTANT   		0
#define CROSSFADE 		1
#define FADE_TO_BLACK	2

// Functions
class Lixie_II
{
	public:
		Lixie_II(const uint8_t pin, uint8_t n_digits);
		void build_controller(const uint8_t pin);
		void begin();
		void max_power(uint8_t V, uint16_t mA);
		void color_all(uint8_t layer, CRGB col);
		void color_all_dual(uint8_t layer, CRGB col_left, CRGB col_right);
		void color_display(uint8_t display, uint8_t layer, CRGB col);
		void gradient_rgb(uint8_t layer, CRGB col_left, CRGB col_right);
		void streak(CRGB col, int16_t pos, uint8_t blur);
		void sweep_color(CRGB col, uint16_t speed, uint8_t blur, bool reverse = false);
		void sweep_gradient(CRGB col_left, CRGB col_right, uint16_t speed, uint8_t blur, bool reverse);
		void start_animation();
		void stop_animation();
		void write(uint32_t input);
		void write_float(float input, uint8_t dec_places = 1);
		void clear_all();
		void write_digit(uint8_t digit, uint8_t num);
		void push_digit(uint8_t number);
		void clear_digit(uint8_t digit, uint8_t num);
		void mask_update();
		
	private:
		uint8_t get_size(uint32_t input);
};

#endif