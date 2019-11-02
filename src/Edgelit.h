/*
	Edgelit.h - Library for controlling any WS2812B-based edgelit
	numeric display, such as the Lixie or NixiePipe.
	
	Created by Connor Nishijma April 17, 2018
	Released under the GPLv3 License
*/

// This stops the biggest fans of my hacky code
// from including it more than once.
#ifndef edgelit_h
#define edgelit_h

// CUSTOM DISPLAY TYPES ARE DEFINED HERE! ------------------------
// ----Check Edgelit.cpp for details!
#define LIXIE_1		0
#define NIXIE_PIPE	1
// CUSTOM DISPLAY TYPES ARE DEFINED HERE! ------------------------ 

// Every library has this. I've never gotten around to
// researching what the hell this is. Better play it safe.
#include "Arduino.h"

// For os_timer_t interrupts
extern "C" {
	#include "user_interface.h"
}

// FastLED has issues with the ESP8266, especially
// when used with networking, so we fix that here.
#define FASTLED_ESP8266_RAW_PIN_ORDER
#define FASTLED_ALLOW_INTERRUPTS 		0
#define FASTLED_INTERRUPT_RETRY_COUNT	0

// Aside from those issues, it's my tool of choice for WS2812B
#include "FastLED.h"

// FastLED info for the LEDs
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB

// Transition types
#define INSTANT			0
#define CROSSFADE		1
#define FADE_TO_BLACK	2

// Layer types
#define FRONT	0
#define BACK	1

// Functions
class Edgelit
{
	public:
		Edgelit(const uint8_t pin, uint8_t nDigits, uint8_t d_type);
		void begin();
		void transition_type(uint8_t type);
		void transition_time(uint16_t ms);
		void write(uint32_t input);
		void push_digit(int16_t number);
		void clear();
		void max_power(uint8_t volts, uint16_t milliamps);
		void nixie(uint8_t argon_intensity = 3);
		void vfd_green(uint8_t aura_intensity = 3);
		void vfd_blue(uint8_t aura_intensity = 3);

		void color(CRGB col, uint8_t layer = 0);
		void color_pixel(uint16_t led, CRGB col, uint8_t layer = 0);
		CRGB color_pixel(uint16_t led, uint8_t layer = 0);

		void color_display(uint16_t disp, CRGB col, uint8_t layer = 0);

		void progress(float percent, float brightness);
		void animation_callback(void (*func)());
		uint16_t led_count();
		void empty_displays(bool state);
		void brightness(float bright);
		void white_balance(CRGB correction);
		
	private:
		uint8_t get_size(uint32_t input) const;
		void build_controller(uint8_t DataPin);		
};

#endif
