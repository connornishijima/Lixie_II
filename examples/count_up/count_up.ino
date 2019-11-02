#include <Lixie_II.h>           // https://github.com/connornishijima/Lixie_II
#define DATA_PIN        13      // Lixie DIN connects to this pin (D7 on Wemos)
#define NUM_DIGITS      4
Lixie_II lix(DATA_PIN, NUM_DIGITS);

uint16_t counter = 0;

void setup() {
	lix.begin();                         // Mandatory, sets up animation timer
	lix.color_all(ON, CRGB(255, 70, 7)); // (Optional) Nixie tube coloring
	lix.transition_time(125);            // (Optional) 125 milliseconds crossfade time 
}

void loop() {
	lix.write(counter);
	delay(1000);
	counter = counter + 1;
}
