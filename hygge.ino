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

  if (EEPROM_RESET || hygge_last.msg_t != 1) {
    // Serial.print("Clearing Last Hygge...");
    hygge_last.seq = 0;
    hygge_last.msg_t = 1;
    hygge_last.hotp = 0;
    hygge_last.batt = 0.0;
    hygge_last.humi = 0.0;
    hygge_last.temp = 0.0;

    EEPROM.put(0, hygge_last);
    if (!EEPROM.commit()) {
      // Serial.println("FAIL");
      // Serial.println("\tError occurred committing cleared struct to EEPROM.");
      bail();
    }

    EEPROM.get(0, hygge_last_c);
    if (hygge_last_c.msg_t != 1) {
      // Serial.println("FAIL");
      // Serial.println("\t!assert(put, read); Did not read what we wrote");
      bail();
    }
    // Serial.println("OK");
  }

  if (EEPROM_RESET) {
    bailSlow();
  }

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
