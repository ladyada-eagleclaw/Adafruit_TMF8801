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
        Serial.println(F(" mm"));
      }
    }
  }
}
