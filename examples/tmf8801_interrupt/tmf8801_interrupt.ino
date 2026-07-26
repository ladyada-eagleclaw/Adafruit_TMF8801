/*!
 * @file tmf8801_interrupt.ino
 *
 * Use the TMF8801 active-low interrupt output to read measurements.
 *
 * Written by Limor Fried/Ladyada for Adafruit Industries.
 * MIT license, all text above must be included in any redistribution.
 */

#include <Adafruit_TMF8801.h>

#define TMF8801_INT_PIN 2

Adafruit_TMF8801 tmf8801;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println(F("Adafruit TMF8801 interrupt test"));

  pinMode(TMF8801_INT_PIN, INPUT_PULLUP);

  if (!tmf8801.begin()) {
    Serial.println(F("Could not find a valid TMF8801 sensor, check wiring!"));
    while (1) {
      delay(10);
    }
  }

  if (!tmf8801.startMeasuring()) {
    Serial.println(F("Could not start measurements"));
    while (1) {
      delay(10);
    }
  }

  Serial.println(F("Waiting for TMF8801 interrupts"));
}
void loop() {
  if (digitalRead(TMF8801_INT_PIN) == LOW) {
    tmf8801_result_t result;
    if (tmf8801.readResult(&result)) {
      if (result.reliability == 0) {
        Serial.println(F("No object detected"));
      } else {
        Serial.print(result.distance_mm);
        Serial.print(F(" mm, reliability: "));
        printReliability(result.reliability);
        Serial.print(F(", status: "));
        printMeasurementStatus(result.status);
        Serial.println();
      }
    }
  }
}

void printReliability(uint8_t reliability) {
  Serial.print(reliability);
  Serial.print(F(" out of "));
  Serial.print(TMF8801_RESULT_RELIABILITY_LEVELS);
  Serial.print(F(" ("));
  if (reliability >= TMF8801_RESULT_RELIABILITY_HIGH_MIN) {
    Serial.print(F("high"));
  } else {
    Serial.print(F("low"));
  }
  Serial.print(F(")"));
}

void printMeasurementStatus(tmf8801_measurement_status_t status) {
  Serial.print((uint8_t)status);
  Serial.print(F(" ("));
  switch (status) {
  case TMF8801_MEASUREMENT_NOT_INTERRUPTED:
    Serial.print(F("measurement not interrupted"));
    break;
  case TMF8801_MEASUREMENT_INTERRUPTED_BY_GPIO:
    Serial.print(F("measurement interrupted by GPIO"));
    break;
  case TMF8801_MEASUREMENT_STATUS_RESERVED_1:
  case TMF8801_MEASUREMENT_STATUS_RESERVED_3:
    Serial.print(F("reserved"));
    break;
  default:
    Serial.print(F("unknown"));
    break;
  }
  Serial.print(F(")"));
}
