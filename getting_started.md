# Getting Started with LIXIE II

## *I know you're excited, but we have a few things to cover to make sure you're ready to run Lixie displays!*

## Assembly

Assembly instructions are found in PDF form here:

[Lixie II Kit Assembly Instructions](http://connor-n.com/lixie/Lixie%20II%20Assembly%20Instructions.pdf)

# THINGS TO NOTE:

## Lixies don't do anything on their own.

The Lixie II display is a bit more cost effective than competitors like the NixiePipe due to it being a "naive" display. It has no onboard controller you can program, you'll need an external microcontroller like the Arduino or ESP8266 to display anything.

## Microcontroller

Any AVR/Arduino or ESP8266/derivative microcontroller can be used to control Lixie displays. There is a Lixie II library available for the Arduino IDE:

### Installing the Lixie II Library

***The Lixie II library relies on [the FastLED library from Daniel Garcia](https://github.com/FastLED/FastLED), so make sure you have that installed as well!*** You can get it via the Arduino Library Manager in your Arduino window by clicking "Sketch > Include Library > Manage Libraries" and searching for "FastLED" in the smaller window that appears. (Just click "Install" for the latest version)

**With Arduino Library Manager: (RECOMMENDED)**

1. Open *Sketch > Include Library > Manage Libraries* in the Arduino IDE.
2. Search for "Lixie_II", and select the latest version.
3. Click the Install button and Arduino will prepare the library and examples for you!

**Manual Install:**

1. Click "Clone or Download" above to get an "Lixie_II-master.zip" file.
2. Extract its contents to the libraries folder in your Arduino sketchbook. ("C:/Users/**YOUR_USERNAME**/Documents/Arduino/libraries" on Windows)
3. Rename the folder from "Lixie_II-master" to "Lixie_II".

When finished, an example folder structure for Windows should be:

    C:\Users\[USERNAME]\Documents\Arduino\libraries\Lixie_II\src\Lixie_II.cpp

## Powering Lixies

Before using Lixies, I highly suggest reading through [Adafruit's "NeoPixel Uberguide"](https://learn.adafruit.com/adafruit-neopixel-uberguide/overview). It goes over many details of how the WS2812/B/S smart-RGB LED works. It's one of the most well-written guides out there. There are 22 of these LEDs in each Lixie digit, so you have to consider the following for your power requirements:

### Lixies are powered (and best controlled) with 5 Volts.

The 22 WS2812B LEDs onboard are nominally powered with 5 volts DC, and expect at least 3.5V (Power voltage\*0.7) on the DIN line to control the pixels. If you have a 3V3-logic microcontroller, you may or may not have to use [a level shifter](https://www.adafruit.com/product/1787). However, I have successfully run Lixies off of a 3.3V Wemos D1 Mini (ESP8266 controller) without needing a level shifter - even with the LEDs powered by the 5 volt line.

A workaround is underpowering the LEDs with a 3.3V power line, but this will only be 66% of the brightness 5V power provides.

### Are you running them in combined colors, or full white?

Showing a number at full white means only 2 of the 22 LEDs are at full power. With a power consumption of 60mA per fully lit LED, that's 120mA at most per Lixie. If you were only displaying in red, green, or blue the consumption would be 40mA, a combination like cyan, magenta or yellow would take 80mA per Lixie.

At this rate, a 6-digit clock at full brightness in white would consume 120mA\*6 = 720mA or 0.72A. This could be powered over USB with a standard USB phone wall charger.

### Are you needing any lightshow effects beyond numbers?

The Lixie library has a few functions like **lix.sweep()** that offer some fancier options for lighting effects. The power requirements get a bit steeper with these. There are two power consumption options:

#### Full Brightness

If you want to allow for any possible lighting scenario, you'll need to have a power supply that can support all 22 LEDs in each digit at full white. That is 60mA per LED, multiplied by 22 LEDs, multiplied by the number of digits in your display. For a 6-digit display flashing every single LED at full-white, you would need a power supply capable of 60mA\*22\*6 = 7,920mA or **7.9A**. You'll probably want the microcontroller powered by the same supply, so add an extra ampere for headroom: **8.9A**. However, most people won't be using Lixie II clocks as a lightbar for their off-road truck, so 1 or 2A supplies should be just fine for most cases. ;) (Plenty of USB phone charging blocks can supply this.)

#### Software Regulation

Lixie's library offers a handy **lix.max_power(*volts*, *milliamps*);** function that you can call in your setup before writing anything in the displays. This uses some quick math to determine if the scene you're writing to the LEDs will overreach your power supply limits, and automatically reduce digit brightness to keep power consumption under the limits of your supply.

For example, if I wanted to run a 6-digit clock in full white off of a computer's USB port, (5V, 500mA max) I would include

    lix.max_power(5,500);
    
in my Arduino setup() function. Normally, a 6-digit white clock would consume 720mA, but with the 500mA limit set it will run at 69.4% of the maximum brightness to keep consumption at 500mA. However, this still means we're running the USB port at its max rating, so limiting it to 400mA would be safer. (55.5% brightness)

This software power limit is designed to only reduce brightness *if the current lighting exceeds the power ratings*. If the formentioned clock was only run in green instead, it would consume 240mA and thus would still run at full brightness under a 500mA regulation.

## Wiring

Lixies are limited to the following Arduino digital pins: (On either AVR/Uno or ESP8266/Wemos controllers)

### 0,2,4,5,12,13

The limits on pins is due to the Arduino compiler being unable to detect which type of ESP8266 breakout you're using, if at all. Between the most common ESP8266 versions - ESP-12, ESP-07, Adafruit Huzzah, NodeMCU, Wemos D1 Mini - these are the pins found on every one and *also* on standard Arduinos. If the library defined a pin not present on your controller, compilation would fail.

In all the example code, the default pin used is Pin 13, whether you're using AVR or ESP8266 microcontrollers. (D7 on Wemos)

Hookup looks like this:

    POWER SUPPLY 5V       ---->   LIXIE 5V
    POWER SUPPLY GND      ---->   LIXIE GND
    MICROCONTROLLER PIN   ---->   LIXIE DIN

Lixies are designed to be daisy-chained, and run from right-to-left. Connecting one display to the next is like this:

    ETC...   <----   LIXIE #2 5V    <----   LIXIE #1 5V          <----              POWER SUPPLY 5V
    ETC...   <----   LIXIE #2 GND   <----   LIXIE #1 GND         <----              POWER SUPPLY GND
    ETC...   <----   LIXIE #2 DIN   <----   LIXIE #1 DOUT <- LIXIE #1 DIN   <----   MICROCONTROLLER PIN
    
If you are using your development board (Arduino/ESP8266) as the power source, make sure to use the **lix.max_power(5,450);** limit we talked about earlier to make sure the displays don't overdraw your computer's USB port during development.

*You can use Dupont female-to-female jumpers to connect the displays, or solder directly to the headers.*

If you have 4 or more displays in the chain, I recommend connecting *BOTH* ends of the chain to the 5V/GND lines of the power supply to avoid having dimmer digits at the end of the chain. (Due to the voltage drop across displays) Without doing so, you may notice the displays on the far end of the chain gradually falling into a more orange-ish color. This is due to the different voltage requirements of the individual red, green, and blue LEDs.

Blue requires the most voltage to light, followed by green then red needing the lowest voltage. If a digit is receiving less than 5 volts, the blue will be the first to drop its performance, then green, then red. If the last display in the chain is supposed to be *white* (equal parts RGB) you'll get something like this instead if not powered from both ends:

Red (full power) + Green (dim) + Blue (darkest) = Orange

## Writing Arduino code for Lixies

Once you've installed the Lixie_II library, go ahead and try this example code to make sure everything is working:

    #include "Lixie_II.h" // Include Lixie Library
    
    #define DATA_PIN   13
    #define NUM_LIXIES 1
    Lixie lix(DATA_PIN, NUM_LIXIES);
    
    uint16_t count = 0;
    
    void setup() {
      lix.begin(); // Initialize LEDs
      lix.max_power(5,450); // Set software power limit
    }
    
    void loop() {
      lix.write(count); // Display count
      delay(100);
      count++;
    }
    
Change the **NUM_LIXIES** variable to match the number of digits you have wired up. When uploaded, you should see a countup from zero running in white!

From here, I suggest trying out the "Introduction Tour" example included with the Lixie_II library, as it covers many of the neat color and timing functions that Lixie II is capable of!
