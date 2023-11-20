/**
 * HYGGE - SENSOR
 *
 * Step 1: Init
 * Step 2: Collect Data
 * Step 3: Process Data
 * Step 4: Transmit Data
 * Step 5: Sleep
 */
#include "hygge.h"

void setup() {
  start_millis = millis();

  setCpuFrequencyMhz(80);

  EEPROM.begin(EEPROM_SIZE);

  // Initialize all board peripherals, call this first
  ums3.begin();

  // Serial.begin(9600);
  // while (!Serial)
  //   delay(1);

  // Turn it off; we'll turn it back on if we're on line power (USB plugged in) for indication
  ums3.setPixelBrightness(0);
  ums3.setPixelPower(false);

  // Get the temp/humi
  initSHT31();
  readSHT31();

  //////////////////
  // DO THE WORK
  checkBattery();

  // Serial.print("Temp = ");
  // Serial.print(hygge.temp, 2);
  // Serial.println(" C");
  // Serial.print("Humi = ");
  // Serial.print(hygge.humi, 2);
  // Serial.println("% rh");

  prepareMsg();

  initRFM95();
  sendMsg();
  
  sleep();
}

void loop() {
  // nothing to see here
}

// Gets the battery voltage and shows it using the neopixel LED.
// These values are all approximate, you should do your own testing and
// find values that work for you.
void checkBattery() {
  // Get the battery voltage, corrected for the on-board voltage divider
  // Full should be around 4.2v and empty should be around 3v
  hygge.batt = ums3.getBatteryVoltage();
  // Serial.print("Batt = ");
  // Serial.print(hygge.batt, 2);
  // Serial.println(" V");

  if (ums3.getVbusPresent()) {
    ums3.setPixelPower(true);
    ums3.setPixelBrightness(255);
    // If USB power is present
    if (hygge.batt < 4.0) {
      // Charging - blue
      ums3.setPixelColor(0x0000FF);
    } else {
      // Close to full - off
      ums3.setPixelColor(0x000000);
    }
  } else {
    // Else, USB power is not present (running from battery)
    if (hygge.batt < 3.71) {
      // Uncomment the following line to sleep when the battery is critically low
      esp_deep_sleep_start();
    }
  }
}

