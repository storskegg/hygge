#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_ThinkInk.h>
#include <Adafruit_SHTC3.h>
#include "rms.h"

#define EPD_DC      7 
#define EPD_CS      8
#define EPD_BUSY   -1
#define SRAM_CS    -1 
#define EPD_RESET   6

#define SPKR_ON HIGH
#define SPKR_OFF LOW
#define PXLS_ON LOW
#define PXLS_OFF HIGH

// 2.9" Grayscale Featherwing, MagTag or Breakout:
ThinkInk_290_Grayscale4_T5 display(EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY);

// The four neopixel display lights
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(4, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

// The temp/humidity sensor
Adafruit_SHTC3 shtc3 = Adafruit_SHTC3();

unsigned long last_sample = 0;
unsigned long last_report = 0;

const unsigned long period_report_seconds = 60 * 10;
const unsigned long period_report_ms = 1000 * period_report_seconds;
const unsigned long period_sample_seconds = 60;
const unsigned long period_sample_ms = 1000 * period_sample_seconds;
const unsigned long samples_per_report = period_report_seconds / period_sample_seconds;

rms_t Temps;
rms_t Humis;

void readButtons(void);
void sample(bool);
void disp(bool);
void sample_and_display(bool);

void pxlOn() {
  digitalWrite(NEOPIXEL_POWER, PXLS_ON);
}

void pxlOff() {
  digitalWrite(NEOPIXEL_POWER, PXLS_OFF);
}

void spkrOn() {
  digitalWrite(SPEAKER_SHUTDOWN, SPKR_ON);
}

void spkrOff() {
  digitalWrite(SPEAKER_SHUTDOWN, SPKR_OFF);
}

void dispOn() {
  // display.powerUp();
  // digitalWrite(EPD_RESET, HIGH);
}

void dispOff() {
  display.powerDown();
  digitalWrite(EPD_RESET, LOW); // hardware power down mode
}
// void ledOn() {}
// void ledOff() {}

void initPins() {
  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);
  pinMode(BUTTON_D, INPUT_PULLUP);
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);

  pinMode(NEOPIXEL_POWER, OUTPUT);
  pinMode(SPEAKER_SHUTDOWN, OUTPUT);

  // Init with them powered down.  
  pxlOff();
  spkrOff();
}

void setup() {
  initPins();

  Wire.begin();
  
  //Initialize serial
  Serial.begin(9600);
  // while (!Serial)
  //   delay(100);

  Serial.print("\nINIT::eInk...");
  display.begin(THINKINK_MONO); // Thin Kink Mono

  display.clearBuffer();
  display.clearDisplay();
  display.setTextSize(3);
  display.setCursor(20, 40);
  display.println("booting...");
  display.display();
  Serial.println("DONE");

  // Neopixel power
  Serial.print("INIT::NeoPixels...");
  digitalWrite(NEOPIXEL_POWER, LOW); // on

  pixels.begin();
  pixels.setBrightness(20);
  pixels.fill(0xFF00FF);
  pixels.show(); // Initialize all pixels to 'off'

  Serial.println("DONE");

  Serial.print("INIT::SHTC3...");
  if (!shtc3.begin()) {
    Serial.println("ERROR -- Couldn't find SHTC3. Halting...");
    while (1) 
      delay(10000);
  }
  Serial.println("DONE");
  
  Serial.print("INIT::Temps...");
  Temps = new_rms();
  Serial.println("DONE");
  Serial.print("INIT::Humis...");
  Humis = new_rms();
  Serial.println("DONE");
  
  delay(2000); // This delay is more to let the display settle, than anything else
  sample_and_display(false);
  delay(2000);

  dispOff();
  spkrOff();
  // pxlOff();
  
  esp_sleep_enable_timer_wakeup(period_report_ms * 1000);
  esp_deep_sleep_start();
}

void loop() {
  // readButtons();

  // if (millis() - last_sample > period_sample_ms) {
  //   sample(true);
  // }

  // if (millis() - last_report > period_report_ms) {
  //   disp(true);
  // }
}

void readButtons() {
  if (! digitalRead(BUTTON_A)) {
    sample_and_display(false);
  }
  else if (! digitalRead(BUTTON_B)) {
    Serial.println("Button B pressed");
  }
  else if (! digitalRead(BUTTON_C)) {
    Serial.println("Button C pressed");
  }
  else if (! digitalRead(BUTTON_D)) {
    digitalWrite(NEOPIXEL_POWER, LOW); // on
    pixels.fill(0xFF0000);
    pixels.show();
  }
  else {
    // No buttons pressed! turn em off
    digitalWrite(NEOPIXEL_POWER, HIGH);
  }
}

void sample(bool resetLast) {
  sensors_event_t humidity, temp;
  
  shtc3.sleep(false);
  shtc3.getEvent(&humidity, &temp);// populate temp and humidity objects with fresh data
  shtc3.sleep(true);
  
  for (size_t i = 0; i < Temps.sz; i++) {
    push(temp.temperature, &Temps);
    push(humidity.relative_humidity, &Humis);

    delay(50);
  }

  if (resetLast)
    last_sample = millis();
}

void disp(bool resetLast) {
  float temp = rms(&Temps);
  float humi = rms(&Humis);

  display.clearBuffer();
  display.setCursor(10, 10);
  display.setTextSize(3);
  display.printf("%.03f C", temp);
  display.setCursor(10, 40);
  display.printf("%.03f %% rh", humi);
  Serial.printf("%.03f C\n%.03f %% rh\n", temp, humi);
  if (humi < 68.0) {
    display.setCursor(10, 90);
    display.print("ALERT! %RH LOW");
  } else if (humi > 76.0) {
    display.setCursor(10, 90);
    display.print("ALERT! %RH HIGH");
  }
  display.display();

  if (resetLast)
    last_report = millis();
}

void sample_and_display(bool resetLast) {
  sample(resetLast);
  disp(resetLast);
}
