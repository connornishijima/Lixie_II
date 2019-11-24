#include <Lixie_II.h>            // https://github.com/connornishijima/Lixie_II

#include <ESP8266WiFi.h>         // https://github.com/esp8266/Arduino
#include <DNSServer.h>           //  |
#include <ESP8266WebServer.h>    //  |
#include <WiFiUdp.h>             //  |
#include <FS.h>                  // <

#include <WiFiManager.h>         // https://github.com/tzapu/WiFiManager
#include <NTPClient.h>           // https://github.com/arduino-libraries/NTPClient
#include <ArduinoJson.h>         // https://github.com/bblanchon/ArduinoJson

/*
   Lixie II NTP Clock for ESP8266
   by Connor Nishijima (November 2nd, 2019)

   This example relies on an ESP8266/ESP32 controller, which is used to pull
   UTC time from an NTP server over WiFi. There are also a few external libraries
   required for NTP timekeeping, which are provided as GitHub links at the top of
   this sketch and can also be installed by name using the Arduino Library Manager.
   
   REQUIRED HARDWARE ---------------------------------------------------------------------

   It requires two external buttons to fully function: (defined as HUE_BUTTON and
   HOUR_BUTTON below) one for changing color/modes, and one for setting the UTC
   offset (timezone)

   INSTRUCTIONS --------------------------------------------------------------------------

   Upon booting, it will failt to connect to WiFi as it doesn't yet know your
   credentials. It will then host it's own WiFi access point named "LIXIE CONFIG"
   that you can connect your phone to.

   Once connected, go to http://192.168.4.1/ in your phone's browser, and you
   will see a menu that will let you send the WiFi details down to the clock,
   to be remembered from this point forward. (Even through power cycles!)

   A short tap of the HUE button will change color modes, and holding it down will
   start to cycle the displays through the color wheel. Release the HUE button
   when it gets to a color you like.

   A short tap of the HOUR button will increase the UTC offset by 1, (wrapping back
   to -12 after +12) and a long tap toggles between 12 and 24-hour mode.

   Holding the HOUR button down during power up will erase WiFi credentials and let
   you access the WiFi configuration page again.
   
   OPTIONAL HARDWARE ---------------------------------------------------------------------

   - A piezo buzzer or small speaker between the BUZZER pin and GND
   - Two SPST / SPDT switches from both the COLOR_CYCLE and NIGHT_DIMMING pins, that
     optionally tie those pins to GND

   The buzzer is just for feedback when buttons are pressed, and the two switches
   enable automated color cycling on modes that support it (full cycle every 5 minutes)
   and nighttime dimming to 40% brightness from 9PM (21:00) to 6AM (06:00)

   If you don't have a buzzer or switches on-hand, just ignore the buzzer and
   short the COLOR_CYCLE / NIGHT_DIMMING pins to GND if you want to enable those features.

   MAKE SURE SPIFFS IS ENABLED in Tools > Flash Size

   Enjoy!
*/

// USER SETTINGS //////////////////////////////////////////////////////////////////
#define DATA_PIN        D6      // Lixie DIN connects to this pin
#define NUM_DIGITS      4

#define SIX_DIGIT_CLOCK false   // 6 or 4-digit clock? (6 has seconds shown)

#define HOUR_BUTTON     D7      // These are pulled up internally, and should be
#define HUE_BUTTON      D2      // tied to GND through momentary switches

#define BUZZER          D8      // OPTIONAL
#define COLOR_CYCLE     D1      // OPTIONAL
#define NIGHT_DIMMING   D5      // OPTIONAL
///////////////////////////////////////////////////////////////////////////////////

Lixie_II lix(DATA_PIN, NUM_DIGITS);
WiFiUDP ntp_UDP;
NTPClient time_client(ntp_UDP, "pool.ntp.org", 3600, 60000);
#define SECONDS_PER_HOUR 3600
bool time_found = false;

struct conf {
  int16_t time_zone_shift = 0;
  uint8_t hour_12_mode = false;
  uint8_t base_hue = 0;
  uint8_t current_mode = 0;

};
conf clock_config; // <- global configuration object

float base_hue_f = 0;
uint32_t settings_last_update = 0;
bool settings_changed = false;
const char* settings_file = "/settings.json";

uint32_t t_now = 0;
uint8_t last_seconds = 0;

#define STABLE 0
#define RISE   1
#define FALL   2

uint8_t hh = 0;
uint8_t mm = 0;
uint8_t ss = 0;

bool color_cycle_state   = HIGH;
bool night_dimming_state = HIGH;

bool hour_button_state   = HIGH;
bool hue_button_state  = HIGH;
bool hour_button_state_last   = HIGH;
bool hue_button_state_last = HIGH;
uint8_t hour_button_edge   = STABLE;
uint8_t hue_button_edge = STABLE;
uint32_t hour_button_last_hit = 0;
uint32_t hue_button_last_hit = 0;
uint32_t hour_button_start = 0;
uint16_t hour_button_wait = 1000;
bool hour_mode_started = false;
uint32_t hue_button_start = 0;
uint8_t button_debounce_ms = 100;

uint16_t hue_countdown = 255;
uint16_t hue_push_wait = 500;

#define NUM_MODES 7

#define MODE_SOLID          0
#define MODE_GRADIENT       1
#define MODE_DUAL           2
#define MODE_NIXIE          3
#define MODE_INCANDESCENT   4
#define MODE_VFD            5
#define MODE_WHITE          6

void setup() {
  Serial.begin(115200);
  init_fs();
  load_settings();

  init_displays();
  init_buttons();
  init_wifi();
  init_ntp();
}

void loop() {
  run_clock();
  yield();
}

void run_clock() {
  t_now = millis();
  time_client.update();

  if (time_client.getSeconds() != last_seconds) {
    show_time();
  }

  if (t_now % 20 == 0) { // 50 FPS
    color_for_mode();
    check_buttons();

    lix.run();
  }

  if (t_now - settings_last_update > 5000 && settings_changed == true) {
    settings_changed = false;
    save_settings();
  }
}

void save_settings() {
  // Delete existing file, otherwise the configuration is appended to the file
  SPIFFS.remove(settings_file);

  // Open file for writing
  File file = SPIFFS.open(settings_file, "w+");
  if (!file) {
    Serial.println(F("Failed to create config file"));
    return;
  }
  else {
    Serial.println("Config file opened");
  }

  // Allocate a temporary JsonDocument
  StaticJsonDocument<512> doc_out;

  // Set the values in the document
  doc_out["base_hue"] = clock_config.base_hue;
  doc_out["current_mode"] = clock_config.current_mode;
  doc_out["time_zone_shift"] = clock_config.time_zone_shift;
  doc_out["hour_12_mode"] = clock_config.hour_12_mode;

  // Serialize JSON to file
  if (serializeJson(doc_out, file) == 0) {
    Serial.println(F("Failed to write to config file"));
  }
  else {
    Serial.println("Config Saved!");
  }

  // Close the file
  file.close();
}

void load_settings() {
  // Open file for reading
  File file = SPIFFS.open(settings_file, "r");

  // Allocate a temporary JsonDocument
  StaticJsonDocument<512> doc_in;

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc_in, file);
  if (error) {
    Serial.println(F("Failed to read file, using default configuration"));
  }
  else {
    Serial.println("Config file opened");
    Serial.println("Config Loaded!");
  }

  // Copy values from the JsonDocument to the Config
  clock_config.base_hue = doc_in["base_hue"];
  base_hue_f            = doc_in["base_hue"];
  clock_config.current_mode = doc_in["current_mode"];
  clock_config.time_zone_shift = doc_in["time_zone_shift"];
  clock_config.hour_12_mode = doc_in["hour_12_mode"];

  // Close the file (Curiously, File's destructor doesn't close the file)
  file.close();
}

void show_time() {
  hh = time_client.getHours();
  mm = time_client.getMinutes();
  ss = time_client.getSeconds();

  if (hh < 6 || hh >= 21) { // Dim overnight from 9PM to 6AM (21:00 to 06:00)
    if (night_dimming_state == HIGH) { // But only if dimming is enabled
      lix.brightness(0.4);
    }
    else { // If not enabled, full brightness
      lix.brightness(1.0);
    }
  }
  else { // Or if not in the overnight time window, full brightness
    lix.brightness(1.0);
  }

  // 12 hour format conversion
  if (clock_config.hour_12_mode == true) {
    if (hh > 12) {
      hh -= 12;
    }
    if (hh == 0) {
      hh = 12;
    }
  }

  uint32_t t_lixie = 1000000; // "1000000" is used to get zero-padding on hours digits
  // This turns a time of 22:34:57 into the integer (1)223457, whose leftmost numeral (1) will not be shown
  t_lixie += (hh * 10000);
  t_lixie += (mm * 100);
  t_lixie += ss;

  if (!SIX_DIGIT_CLOCK) {
    t_lixie /= 100; // Eliminate second places if using a 4 digit clock
  }

  lix.write(t_lixie); // Update numerals
  
  if (!time_found) { // Cues initial fade in
    time_found = true;
    lix.fade_in();
  }

  Serial.print("TIME: ");
  Serial.println((hh * 10000)+(mm * 100)+ss);

  last_seconds = ss;
}

void color_for_mode() {
  if (color_cycle_state == HIGH) {
    base_hue_f += 0.017; // Fully cycles the color wheel every 5 minutes
  }

  clock_config.base_hue = base_hue_f;
  uint8_t temp_hue = clock_config.base_hue + hue_countdown;

  if (clock_config.current_mode == MODE_SOLID) {
    lix.color_all(ON, CHSV(temp_hue, 255, 255));
    lix.color_all(OFF, CRGB(0, 0, 0));
  }
  else if (clock_config.current_mode == MODE_GRADIENT) {
    lix.gradient_rgb(ON, CHSV(temp_hue, 255, 255), CHSV(temp_hue + 90, 255, 255));
    lix.color_all(OFF, CRGB(0, 0, 0));
  }
  else if (clock_config.current_mode == MODE_DUAL) {
    lix.color_all_dual(ON, CHSV(temp_hue, 255, 255), CHSV(temp_hue + 90, 255, 255));
    lix.color_all(OFF, CRGB(0, 0, 0));
  }
  else if (clock_config.current_mode == MODE_NIXIE) {
    lix.color_all(ON, CRGB(255, 70, 7));
    CRGB col_off = CRGB(0, 100, 255);
    const uint8_t nixie_aura_level = 8;

    col_off.r *= (nixie_aura_level / 255.0);
    col_off.g *= (nixie_aura_level / 255.0);
    col_off.b *= (nixie_aura_level / 255.0);

    lix.color_all(OFF, col_off);
  }
  else if (clock_config.current_mode == MODE_INCANDESCENT) {
    lix.color_all(ON, CRGB(255, 100, 25));
    lix.color_all(OFF, CRGB(0, 0, 0));
  }
  else if (clock_config.current_mode == MODE_VFD) {
    lix.color_all(ON, CRGB(100, 255, 100));
    lix.color_all(OFF, CRGB(0, 0, 0));
  }
  else if (clock_config.current_mode == MODE_WHITE) {
    lix.color_all(ON, CRGB(255, 255, 255));
  }
}

void check_buttons() {
  hour_button_state   = digitalRead(HOUR_BUTTON);
  hue_button_state  = digitalRead(HUE_BUTTON);

  color_cycle_state  = !digitalRead(COLOR_CYCLE);
  night_dimming_state  = !digitalRead(NIGHT_DIMMING);

  if (hour_button_state > hour_button_state_last) {
    hour_button_edge = RISE;
  }
  else if (hour_button_state < hour_button_state_last) {
    if (t_now - hour_button_last_hit >= button_debounce_ms) {
      hour_button_last_hit = t_now;
      hour_button_edge = FALL;
    }
  }
  else {
    hour_button_edge = STABLE;
  }

  if (hue_button_state > hue_button_state_last) {
    hue_button_edge = RISE;
  }
  else if (hue_button_state < hue_button_state_last) {
    if (t_now - hue_button_last_hit >= button_debounce_ms) {
      hue_button_last_hit = t_now;
      hue_button_edge = FALL;
    }
  }
  else {
    hue_button_edge = STABLE;
  }

  parse_buttons();

  hour_button_state_last   = hour_button_state;
  hue_button_state_last  = hue_button_state;
}

void parse_buttons() {
  if (hour_button_edge == FALL) { // PRESS STARTED
    hour_button_start = t_now;
  }
  else if (hour_button_edge == RISE) { // PRESS ENDED
    uint16_t hour_button_duration = t_now - hour_button_start;
    if (hour_button_duration < hour_button_wait) { // RELEASED QUICKLY
      Serial.println("UP");
      clock_config.time_zone_shift += 1;

      if (clock_config.time_zone_shift >= 12) {
        clock_config.time_zone_shift = -12;
      }

      time_client.setTimeOffset(clock_config.time_zone_shift * SECONDS_PER_HOUR);
      hh = time_client.getHours();
      if (hh == 0) {
        beep(1000, 100);
      }
      else {
        beep(2000, 100);
      }
      show_time();
      update_settings();
    }
    else { // RELEASED AFTER LONG PRESS
      hour_mode_started = false;
    }
  }

  if (hue_button_edge == FALL) { // PRESS STARTED
    Serial.println("HUE");
    hue_button_start = t_now;
  }
  else if (hue_button_edge == RISE) { // PRESS ENDED
    uint16_t hue_button_duration = t_now - hue_button_start;
    if (hue_button_duration < hue_push_wait) { // RELEASED QUICKLY
      Serial.println("NEXT MODE");
      clock_config.current_mode++;
      if (clock_config.current_mode >= NUM_MODES) {
        clock_config.current_mode = 0;
        beep(1000, 100);
      }
      else {
        beep(2000, 100);
      }
      hue_countdown = 127;
      update_settings();
    }
  }

  if (hue_button_state == LOW) { // CURRENTLY PRESSING
    uint16_t hue_button_duration = t_now - hue_button_start;
    if (hue_button_duration >= hue_push_wait) {
      base_hue_f++;
      Serial.print("HUE: ");
      Serial.println(base_hue_f);
      update_settings();
    }
  }

  if (hour_button_state == LOW) { // CURRENTLY PRESSING
    uint16_t hour_button_duration = t_now - hour_button_start;
    if (hour_button_duration >= hour_button_wait && hour_mode_started == false) {
      hour_mode_started = true;
      Serial.println("CHANGE HOUR MODE");
      beep(4000, 250);
      clock_config.hour_12_mode = !clock_config.hour_12_mode;
      update_settings();
    }
  }

  if (hue_countdown < 255) {
    hue_countdown += 6;
    if (hue_countdown > 255) {
      hue_countdown = 255;
    }
  }
}

void update_settings() {
  settings_changed = true;
  settings_last_update = t_now;
}

void enter_config_mode() {
  lix.write(808080);
  lix.color_all(ON, CRGB(64, 0, 255));
  beep_dual(2000, 1000, 500);
}

void config_mode_callback (WiFiManager *myWiFiManager) {
  enter_config_mode();
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void init_wifi() {
  WiFiManager wifiManager;
  wifiManager.setAPCallback(config_mode_callback);
  wifiManager.setTimeout(300); // Five minutes

  if (digitalRead(HOUR_BUTTON) == LOW) {
    wifiManager.resetSettings();
    enter_config_mode();
  }
  else {
    beep(2000, 100);
  }

  while (!wifiManager.autoConnect("LIXIE CONFIG")) {
    lix.sweep_color(CRGB(0, 255, 127), 20, 3, false);
    lix.clear(); // Removes "8"s before fading in the time
  }

  beep_dual(1000, 2000, 100);
  lix.sweep_color(CRGB(0, 255, 127), 20, 3, false);
  lix.clear(); // Removes "8"s before fading in the time
  color_for_mode();
}

void init_ntp() {
  time_client.begin();
  time_client.setTimeOffset(clock_config.time_zone_shift * SECONDS_PER_HOUR);
}

void init_fs() {
  Serial.print("SPIFFS Initialize....");
  if (SPIFFS.begin()) {
    Serial.println("ok");
  }
  else {
    Serial.println("failed");
  }
}

void init_buttons() {
  pinMode(HOUR_BUTTON, INPUT_PULLUP);
  pinMode(HUE_BUTTON,  INPUT_PULLUP);

  pinMode(BUZZER, OUTPUT);

  pinMode(COLOR_CYCLE,  INPUT_PULLUP);
  pinMode(NIGHT_DIMMING,  INPUT_PULLUP);
}

void init_displays() {
  lix.begin();
  lix.max_power(5, 1000);
  lix.write(888888);
}

void beep(uint16_t freq, uint16_t len) {
  uint32_t period = (F_CPU / freq) / 2;
  uint32_t cycle_ms = F_CPU / 1000;
  uint32_t t_start = ESP.getCycleCount();
  uint32_t last_flip = t_start;
  uint32_t t_end = t_start + (len * cycle_ms);
  uint32_t t_now = t_start;
  bool state = LOW;
  while (t_now < t_end) {
    t_now = ESP.getCycleCount();
    if (t_now - last_flip >= period) {
      last_flip += period;
      state = !state;

      if (state) {
        GPOS = (1 << BUZZER);
      }
      else {
        GPOC = (1 << BUZZER);
      }
    }
  }
  digitalWrite(BUZZER, LOW);
}

void beep_dual(uint16_t del1, uint16_t del2, uint16_t len) {
  beep(del1, len);
  beep(del2, len);
}
