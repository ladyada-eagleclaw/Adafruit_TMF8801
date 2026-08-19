/*!
 * @file tmf8801_singleshot.ino
 *
 * Take one TMF8801 distance measurement at a time.
 *
 * Written by Limor Fried/Ladyada for Adafruit Industries.
 * MIT license, all text above must be included in any redistribution.
 */

#include <Adafruit_TMF8801.h>

const uint16_t MEASUREMENT_TIMEOUT_MS = 1000;

Adafruit_TMF8801 tmf8801;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println(F("Adafruit TMF8801 single-shot test"));

  if (!tmf8801.begin()) {
    Serial.println(F("Could not find a valid TMF8801 sensor, check wiring!"));
    while (1) {
      delay(10);
    }
  }
  Serial.println(F("TMF8801 found"));

  tmf8801.setIterations(900); // 900,000 integration iterations
}

void loop() {
  // Passing false requests one result instead of continuous measurements. Call
  // startMeasuring(false) again whenever another reading is needed.
  if (!tmf8801.startMeasuring(false)) {
    Serial.println(F("Could not start a single-shot measurement"));
    delay(1000);
    return;
  }

  uint32_t start = millis();
  while (!tmf8801.dataReady() &&
         (millis() - start < MEASUREMENT_TIMEOUT_MS)) {
    delay(1);
  }
  if (!tmf8801.dataReady()) {
    Serial.println(F("Single-shot measurement timed out"));
    delay(1000);
    return;
  }

  tmf8801_result_t result;
  if (!tmf8801.readResult(&result)) {
    Serial.println(F("Could not read the single-shot result"));
    delay(1000);
    return;
  }

  if (result.reliability == 0) {
    Serial.println(F("No object detected"));
  } else {
    Serial.print(F("Distance: "));
    Serial.print(result.distance_mm);
    Serial.print(F(" mm   Reliability: "));
    printReliability(result.reliability);
    Serial.print(F("   Status: "));
    printMeasurementStatus(result.status);
    Serial.println();
  }

  delay(1000);
}

void printReliability(uint8_t reliability) {
  Serial.print(reliability);
  Serial.print(F("/"));
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
    Serial.print(F("normal"));
    break;
  case TMF8801_MEASUREMENT_INTERRUPTED_BY_GPIO:
    Serial.print(F("interrupted by GPIO"));
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
