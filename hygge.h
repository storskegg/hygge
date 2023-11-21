#include <cstdint>
#include "SimpleHMAC.h"
#include "esp32-hal.h"
#include <sys/_stdint.h>
#pragma once

#ifndef HYGGE_H_

#include <sys/_types.h>
#include <Arduino.h>
#include <SPI.h>
#include <RH_RF95.h>
#include <Wire.h>
#include <EEPROM.h>
#include "esp_sleep.h"
#include <UMS3.h>

#define EEPROM_SIZE 256
#define EEPROM_RESET 0 // Clear reset whatever was in the EEPROM with zero'd hygge_t on first start

UMS3 ums3;

char my_callsign[] = "W4PHO";
unsigned char secret[] = "i am so secret";
bool enableHeater = false;
uint64_t packetnum = 0;

void doHMAC(void);
void initProtos(void);
void prepareMsg(void);
void initSHT31(void);
void readSHT31(void);
void initRFM95(void);
void sendMsg(void);
void sleepRFM95(void);
void sleep(void);
void bail(void);
void bailSlow(void);
void checkBattery(void);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Cadence

const unsigned long period_report_seconds = 15;
const unsigned long period_report_ms = period_report_seconds * 1000;

unsigned long start_millis = 0;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Message

#include <SimpleHOTP.h>

Key key(secret, sizeof(secret)-1);
uint32_t code[5];

typedef struct {
  uint64_t seq;
  uint32_t msg_t;
  uint32_t hotp;
  float batt;
  float temp;
  float humi;
} hygge_t;

hygge_t hygge;
hygge_t hygge_last;
hygge_t hygge_last_c;

char buf_last_hygge[RH_RF95_MAX_MESSAGE_LEN];
int sz_last_hygge = 0;

char buf_pre_hotp[RH_RF95_MAX_MESSAGE_LEN];
int sz_pre_buf = 0;
char buf_post_hotp[RH_RF95_MAX_MESSAGE_LEN];
int sz_post_buf = 0;

void prepareMsg(void) {
  hygge.msg_t = 1;
  
  EEPROM.get(0, hygge_last);
  sz_last_hygge = sprintf(buf_last_hygge, "%s|%lu|%08lx|%lld|%1.2f|%1.2f|%1.2f", my_callsign, hygge_last.msg_t, hygge_last.hotp, hygge_last.seq, hygge_last.humi, hygge_last.temp, hygge_last.batt);
  // Serial.print("Last Hygge: '");
  // Serial.print(buf_last_hygge);
  // Serial.println("'");
  
  hygge.seq = hygge_last.seq + 1;

  sz_pre_buf = sprintf(buf_pre_hotp, "%s|%lu|%lld|%1.2f|%1.2f|%1.2f", my_callsign, hygge.msg_t, hygge.seq, hygge.humi, hygge.temp, hygge.batt);
  // Serial.print("Pre-Buf:  '");
  // Serial.print(buf_pre_hotp);
  // Serial.println("'");

  doHMAC();

  sz_post_buf = sprintf(buf_post_hotp, "%s|%lu|%08lx|%lld|%1.2f|%1.2f|%1.2f", my_callsign, hygge.msg_t, hygge.hotp, hygge.seq, hygge.humi, hygge.temp, hygge.batt);
  // Serial.print("Post-Buf: '");
  // Serial.print(buf_post_hotp);
  // Serial.println("'");
}

void doHMAC(void) {
  SimpleHMAC::generateHMAC(key, (uint8_t*)buf_pre_hotp, 8*(sizeof(buf_pre_hotp) - 1), code);
  // Typically, you want all the bits. But, we prioritize smaller packet for power savings, especially since this isn't critical infrastructure. These 32 bits will be just fine for our use.
  hygge.hotp = code[0];
  // Serial.print("HOTP: ");
  // Serial.print(hygge.hotp, HEX);
  // for (int i = 0; i < 5; i++) {
  //   Serial.print(code[i], HEX);
  //   Serial.print(" ");
  // }
  // Serial.println();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SHT31 (Temp/Humi Sensor)

#include <Adafruit_SHT31.h>

Adafruit_SHT31 sht31 = Adafruit_SHT31();

void initSHT31(void) {
  // Serial.print(":: DO INIT SHT31d...");
  // Init the SHT31d
  ums3.setLDO2Power(true);// we have to enable ldo2 by hand to pull a reading while on batt power
  delay(50); // TODO: tune this. can we bring it in a bit?

  int j = 5;
  do {
    if (sht31.begin(0x44)) {
      j = 5; // set it above 0 in case we succeed on the last try
      break;
    }
    // Serial.print(".");
    delay(10);
    sht31.reset();
    delay(10);
    j--;
  } while (j > 0);

  if (j == 0) {
    // Serial.print("ERROR - 0x");
    // Serial.println(sht31.readStatus(), HEX);
    bail();
  }
  // Serial.print("OK - 0x");
  // Serial.println(sht31.readStatus(), HEX);
}

void readSHT31(void) {
  int err = 0;

  // Serial.print(":: DO READ SHT31d...");
  if (!sht31.readBoth(&hygge.temp, &hygge.humi)) {
    // Serial.print("\n\t\tERROR READ - 0x");
    // Serial.print(sht31.readStatus(), HEX);
    err++;
  }

  ums3.setLDO2Power(false); // and turn the power back off
  
  if (isnan(hygge.temp)) {
    // Serial.print("\n\t\tERROR TEMP - 0x");
    // Serial.print(sht31.readStatus(), HEX);
    hygge.temp = -11.2233;
    err++;
  }

  if (isnan(hygge.humi)) {
    // Serial.print("\n\t\tERROR HUMI - 0x");
    // Serial.print(sht31.readStatus(), HEX);
    hygge.humi = -11.2233;
    err++;
  }
  // if (err == 0) {
  //   Serial.println("OK");
  // } else {
  //   Serial.println();
  // }

  return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RFM9x

// UM esp32-S3 Feather w/ LoRa 433MHz Wing
#define RFM95_RST 1  // RST -> C
#define RFM95_CS  38 // CS -> D
#define RFM95_INT 33 // IRQ -> E

#define RFM95_FREQ 433.0

RH_RF95 rfm95(RFM95_CS, RFM95_INT);

void initRFM95(void) {
  // Serial.print(":: DO INIT RFM95...");

  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);
  delay(100);

  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  while (!rfm95.init()) {
    // Serial.println("ERROR - init()");
    bail();
  }

  if (!rfm95.setFrequency(RFM95_FREQ)) {
    // Serial.println("ERROR - freq");
    bail();
  }

  rfm95.setTxPower(0, false);
  rfm95.setLowDatarate();
  rfm95.setModeIdle();
  // Serial.println("OK");
}

void sendMsg(void) {
  // Serial.println("<<<<<<<<<<<<<<<< [ Raw Packet ] <<<<<<<<<<<<<<<");
  // Serial.println(buf_post_hotp);
  // Serial.println(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
  // Serial.println();
  // Serial.print(":: DO MSG SEND...");
  rfm95.setModeTx();
  rfm95.send((uint8_t*)buf_post_hotp, sizeof(buf_post_hotp));
  delay(10);
  rfm95.waitPacketSent();
  delay(10);
  rfm95.setModeIdle();
  EEPROM.put(0, hygge);
  EEPROM.commit();
  // Serial.println("OK");
}

void sleepRFM95(void) {
  rfm95.sleep();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sleep

void sleep(void) {
  ////////////////////////////////
  // BRING IT DOWN SOFTLY, NOW
  // delay(10000);
  
  // sleep the LoRa radio (power savings, should drop ~2mA)
  delay(10);
  sleepRFM95();
  delay(100);
  
  // Slow it down (power savings)
  setCpuFrequencyMhz(10);

  // We'll deep sleep our cadence minus the difference between now and when we started to keep our cadence right-ish
  esp_sleep_enable_timer_wakeup((period_report_ms - (millis() - start_millis)) * 1000);
  esp_deep_sleep_start();

  // We should now be ~150 uA
}

void bail(void) {
  // Serial.println();
  // Serial.println("!!!!!!!!!!!! WE HAVE BAILED !!!!!!!!!!!!!");

  while (1) {
    ums3.setBlueLED(true);
    delay(250);
    ums3.setBlueLED(false);
    delay(250);
    checkBattery();
  }
}

void bailSlow(void) {
  while (1) {
    ums3.setBlueLED(true);
    delay(500);
    ums3.setBlueLED(false);
    delay(500);
    checkBattery();
  }
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
    if (hygge.batt < 3.72) {
      // Uncomment the following line to sleep when the battery is critically low
      esp_deep_sleep_start();
    }
  }
}

#endif
