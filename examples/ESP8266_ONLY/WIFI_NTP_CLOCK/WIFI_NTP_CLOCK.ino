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
 * Lixie II NTP Clock Example for ESP8266
 * by Connor Nishijima (November 2nd, 2019)
 * 
 * This example relies on an ESP8266/ESP32 controller,
 * which is used to pull UTC time from an NTP server over WiFi.
 * 
 * It requires two external buttons to fully function: (defined as HUE_BUTTON and HOUR_BUTTON below)
 * one for changing color/modes, and one for setting the UTC offset (timezone)
 * 
 * Upon booting, it will failt to connect to WiFi as it doesn't yet know your
 * credentials. It will then host it's own WiFi access point named "LIXIE CONFIG"
 * that you can connect your phone to.
 * 
 * Once connected, go to http://192.168.4.1 in your phone's browser, and you
 * will see a menu that will let you send the WiFi details down to the clock,
 * to be remembered from this point forward. (Even through power cycles!)
 * 
 * A short tap of the HUE button will change color modes, and holding it down will
 * start to cycle the displays through the color wheel. Release the HUE button
 * when it gets to a color you like.
 * 
 * A short tap of the HOUR button will increase the UTC offset by 1, (wrapping back
 * to -12 after +12) and a long tap toggles between 12 and 24-hour mode.
 * 
 * Holding the HOUR button down during power up will let you access the WiFi 
 * configuration page again.
 * 
 * MAKE SURE SPIFFS IS ENABLED in Tools > Flash Size
 * 
 * Enjoy!
 */

// USER SETTINGS //////////////////////////////////////////////////////////////////
#define DATA_PIN        D7      // Lixie DIN connects to this pin
#define NUM_DIGITS      4

#define SIX_DIGIT_CLOCK false   // 6 or 4-digit clock? (6 has seconds shown)

#define HOUR_BUTTON       D1      // These are pulled up internally, and should be
#define HUE_BUTTON      D3      // tied to GND through momentary switches
///////////////////////////////////////////////////////////////////////////////////

Lixie_II lix(DATA_PIN, NUM_DIGITS);
WiFiUDP ntp_UDP;
NTPClient time_client(ntp_UDP, "pool.ntp.org", 3600, 60000);
#define SECONDS_PER_HOUR 3600
int16_t time_zone_shift = 0;
uint8_t hour_12_mode = false;
bool time_found = false;

uint32_t settings_last_update = 0;
bool settings_changed = false;
const char* settings_file = "/settings.json";

uint32_t t_now = 0;
uint8_t last_seconds = 0;

#define STABLE 0
#define RISE   1
#define FALL   2
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

uint8_t base_hue = 0;
uint16_t hue_countdown = 255;
uint16_t hue_push_wait = 500;

uint8_t current_mode = 0;

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
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();

  root["base_hue"] = base_hue;
  root["mode"] = current_mode;
  root["offset"] = time_zone_shift;
  root["hour_mode"] = hour_12_mode;
  
  String settings;
  root.printTo(settings);
  Serial.println(settings);

  File f = SPIFFS.open(settings_file, "w+");

  if (!f) {
    Serial.println("settings file open failed");
  }
  else {
    //Write data to file
    f.print(settings);
    f.close();  //Close file
  }
}

void load_settings() {
  String input = "";

  //Read File data
  File f = SPIFFS.open(settings_file, "r");
  if (!f) {
    Serial.println("settings file open failed");
  }
  else {
    for (uint16_t i = 0; i < f.size(); i++) {
      input += (char)f.read();
    }
    f.close();  //Close file
  }

  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(input);

  base_hue = root["base_hue"];
  current_mode = root["mode"];
  time_zone_shift = root["offset"];
  hour_12_mode = root["hour_mode"];
}

void show_time() {
  uint8_t hh = time_client.getHours();
  uint8_t mm = time_client.getMinutes();
  uint8_t ss = time_client.getSeconds();

  if (hour_12_mode == true) {
    if (hh > 12) {
      hh -= 12;
    }
    if (hh == 0) {
      hh = 12;
    }
  }

  uint32_t t_lixie = 1000000; // "1000000" is used to get zero-padding on hours digits
  // This turns a time of 22:34:57 into the integer (1)223457, whose leftmost numeral will not be shown
  t_lixie += (hh * 10000);
  t_lixie += (mm * 100);
  t_lixie += ss;

  if (!SIX_DIGIT_CLOCK) {
    t_lixie /= 100; // Eliminate second places
  }

  lix.write(t_lixie);
  if (!time_found) {
    time_found = true;
    lix.fade_in();
  }
  Serial.println(t_lixie);

  last_seconds = ss;
}

void color_for_mode() {
  uint8_t temp_hue = base_hue + hue_countdown;

  if (current_mode == MODE_SOLID) {
    lix.color_all(ON, CHSV(temp_hue, 255, 255));
    lix.color_all(OFF, CRGB(0, 0, 0));
  }
  else if (current_mode == MODE_GRADIENT) {
    lix.gradient_rgb(ON, CHSV(temp_hue, 255, 255), CHSV(temp_hue + 90, 255, 255));
    lix.color_all(OFF, CRGB(0, 0, 0));
  }
  else if (current_mode == MODE_DUAL) {
    lix.color_all_dual(ON, CHSV(temp_hue, 255, 255), CHSV(temp_hue + 90, 255, 255));
    lix.color_all(OFF, CRGB(0, 0, 0));
  }
  else if (current_mode == MODE_NIXIE) {
    lix.color_all(ON, CRGB(255, 70, 7));
    CRGB col_off = CRGB(0, 100, 255);
    const uint8_t nixie_aura_level = 8;

    col_off.r *= (nixie_aura_level / 255.0);
    col_off.g *= (nixie_aura_level / 255.0);
    col_off.b *= (nixie_aura_level / 255.0);

    lix.color_all(OFF, col_off);
  }
  else if (current_mode == MODE_INCANDESCENT) {
    lix.color_all(ON, CRGB(255, 100, 25));
    lix.color_all(OFF, CRGB(0, 0, 0));
  }
  else if (current_mode == MODE_VFD) {
    lix.color_all(ON, CRGB(100, 255, 100));
    lix.color_all(OFF, CRGB(0, 0, 0));
  }
  else if (current_mode == MODE_WHITE) {
    lix.color_all(ON, CRGB(255, 255, 255));
  }
}

void check_buttons() {
  hour_button_state   = digitalRead(HOUR_BUTTON);
  hue_button_state  = digitalRead(HUE_BUTTON);

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
      time_zone_shift += 1;

      if(time_zone_shift >= 12){
        time_zone_shift = -12;
      }
      
      time_client.setTimeOffset(time_zone_shift * SECONDS_PER_HOUR);
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
      current_mode++;
      if (current_mode >= NUM_MODES) {
        current_mode = 0;
      }
      hue_countdown = 127;
      update_settings();
    }
  }

  if (hue_button_state == LOW) { // CURRENTLY PRESSING
    uint16_t hue_button_duration = t_now - hue_button_start;
    if (hue_button_duration >= hue_push_wait) {
      base_hue++;
      Serial.print("HUE: ");
      Serial.println(base_hue);
      update_settings();
    }
  }

  if (hour_button_state == LOW) { // CURRENTLY PRESSING
    uint16_t hour_button_duration = t_now - hour_button_start;
    if (hour_button_duration >= hour_button_wait && hour_mode_started == false) {
      hour_mode_started = true;
      Serial.println("CHANGE HOUR MODE");
      hour_12_mode = !hour_12_mode;
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

void init_wifi() {
  WiFiManager wifiManager;
  if (digitalRead(HOUR_BUTTON) == LOW) {
    wifiManager.resetSettings();
    lix.write(808080);
    lix.color_all(ON, CRGB(64,0,255));
  }

  wifiManager.autoConnect("LIXIE CONFIG");
  lix.sweep_color(CRGB(0, 255, 127), 20, 3, false);
  lix.clear(); // Removes "8"s before fading in the time
  color_for_mode();
}

void init_ntp() {
  time_client.begin();
  time_client.setTimeOffset(time_zone_shift * SECONDS_PER_HOUR);
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
  pinMode(HOUR_BUTTON,   INPUT_PULLUP);
  pinMode(HUE_BUTTON,  INPUT_PULLUP);
}

void init_displays() {
  lix.begin();
  lix.max_power(5, 1000);
  lix.write(888888);
}
