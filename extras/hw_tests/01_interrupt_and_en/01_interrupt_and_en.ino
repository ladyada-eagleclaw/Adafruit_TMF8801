/*!
 * TMF8801 interrupt and enable-pin hardware test
 *
 * Wiring:
 *   TMF8801 EN  -> Metro Mini D3
 *   TMF8801 INT -> Metro Mini D2
 */

#include <Adafruit_TMF8801.h>

#define TMF8801_EN_PIN 3
#define TMF8801_INT_PIN 2

Adafruit_TMF8801 tmf8801;

void clearOutputsAndHalt(const __FlashStringHelper* message) {
  digitalWrite(TMF8801_EN_PIN, LOW);
  Serial.println(message);
  while (1) {
    delay(10);
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  pinMode(TMF8801_EN_PIN, OUTPUT);
  pinMode(TMF8801_INT_PIN, INPUT_PULLUP);
  digitalWrite(TMF8801_EN_PIN, HIGH);
  delay(10);

  Serial.println(F("TMF8801 INT and EN hardware test"));

  if (!tmf8801.begin()) {
    clearOutputsAndHalt(F("FAIL: first begin failed"));
  }
  Serial.println(F("PASS: first begin succeeded"));

  if (!tmf8801.startMeasuring()) {
    clearOutputsAndHalt(F("FAIL: measurement start failed"));
  }
  Serial.println(F("PASS: measurement started"));

  uint32_t start = millis();
  while (digitalRead(TMF8801_INT_PIN) == HIGH &&
         (millis() - start < 1000)) {
    delay(1);
  }
  if (digitalRead(TMF8801_INT_PIN) != LOW) {
    clearOutputsAndHalt(F("FAIL: INT did not assert low"));
  }
  Serial.println(F("PASS: INT asserted low"));

  tmf8801_result_t result;
  if (!tmf8801.readResult(&result)) {
    clearOutputsAndHalt(F("FAIL: result read failed"));
  }
  Serial.println(F("PASS: result read and interrupt cleared"));

  digitalWrite(TMF8801_EN_PIN, LOW);
  delay(100);
  if (digitalRead(TMF8801_EN_PIN) != LOW) {
    clearOutputsAndHalt(F("FAIL: Metro D3 did not drive low"));
  }
  Serial.println(F("PASS: Metro D3 drove low"));
  Wire.beginTransmission(TMF8801_DEFAULT_ADDR);
  if (Wire.endTransmission() == 0) {
    clearOutputsAndHalt(F("FAIL: sensor responded with EN low"));
  }
  Serial.println(F("PASS: sensor disabled with EN low"));

  digitalWrite(TMF8801_EN_PIN, HIGH);
  delay(10);
  if (!tmf8801.begin()) {
    clearOutputsAndHalt(F("FAIL: second begin failed"));
  }
  Serial.println(F("PASS: second begin succeeded"));
  Serial.println(F("ALL TESTS PASSED"));
}

void loop() { delay(1000); }
