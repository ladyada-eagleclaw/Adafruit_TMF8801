/*!
 * @file tmf8801_simpletest.ino
 *
 * Basic continuous-ranging example for the Adafruit TMF8801.
 *
 * Written by Limor Fried/Ladyada for Adafruit Industries.
 * MIT license, all text above must be included in any redistribution.
 */

#include <Adafruit_TMF8801.h>

Adafruit_TMF8801 tmf8801;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println(F("Adafruit TMF8801 test"));

  if (!tmf8801.begin()) {
    Serial.println(F("Could not find a valid TMF8801 sensor, check wiring!"));
    while (1) {
      delay(10);
    }
  }
  Serial.println(F("TMF8801 found"));

  uint8_t major = 0;
  uint8_t minor = 0;
  uint8_t patch = 0;
  tmf8801.getVersion(&major, &minor, &patch);
  Serial.print(F("Firmware version: "));
  Serial.print(major);
  Serial.print(F("."));
  Serial.print(minor);
  Serial.print(F("."));
  Serial.println(patch);

  tmf8801.setIterations(900);       // 900,000 integration iterations
  tmf8801.setRepetitionPeriod(33); // About 30 measurements per second

  if (!tmf8801.startMeasuring()) {
    Serial.println(F("Could not start measurements"));
    while (1) {
      delay(10);
    }
  }
}

void loop() {
  if (tmf8801.dataReady()) {
    tmf8801_result_t result;
    if (tmf8801.readResult(&result)) {
      if (result.reliability == 0) {
        Serial.println(F("No object detected"));
      } else {
        Serial.print(F("Distance: "));
        Serial.print(result.distance_mm);
        Serial.print(F(" mm\tReliability: "));
        Serial.print(result.reliability);
        Serial.print(F("\tStatus: "));
        Serial.println(result.status);
      }
    }
  }
}
