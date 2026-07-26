/*!
 * TMF8801 basic hardware test
 *
 * Wiring:
 *   TMF8801 EN  -> Metro Mini D3
 *   TMF8801 INT -> Metro Mini D2
 */

#include <Adafruit_TMF8801.h>

#define TMF8801_EN_PIN 3
#define TMF8801_INT_PIN 2

Adafruit_TMF8801 tmf8801;

uint8_t readRegister(uint8_t reg) {
  Wire.beginTransmission(TMF8801_DEFAULT_ADDR);
  Wire.write(reg);
  Wire.endTransmission();
  Wire.requestFrom(TMF8801_DEFAULT_ADDR, 1);
  return Wire.read();
}

void printRegister(uint8_t reg) {
  Serial.print(F("Register 0x"));
  Serial.print(reg, HEX);
  Serial.print(F(" = 0x"));
  Serial.println(readRegister(reg), HEX);
}

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

  Serial.println(F("TMF8801 basic hardware test"));

  if (!tmf8801.begin()) {
    clearOutputsAndHalt(F("FAIL: begin failed"));
  }
  Serial.println(F("PASS: begin succeeded"));

  if (tmf8801.getChipID() != TMF8801_CHIP_ID) {
    clearOutputsAndHalt(F("FAIL: chip ID did not match TMF8801_CHIP_ID"));
  }
  Serial.println(F("PASS: chip ID matched TMF8801_CHIP_ID"));

  uint16_t serialNumber;
  if (!tmf8801.readSerialNumber(&serialNumber)) {
    clearOutputsAndHalt(F("FAIL: serial number read failed"));
  }
  Serial.print(F("PASS: serial number read succeeded: 0x"));
  Serial.println(serialNumber, HEX);

  if (!tmf8801.startMeasuring()) {
    printRegister(TMF8801_REG_COMMAND);
    printRegister(TMF8801_REG_PREVIOUS);
    printRegister(TMF8801_REG_STATE);
    printRegister(TMF8801_REG_STATUS);
    printRegister(TMF8801_REG_CONTENTS);
    printRegister(TMF8801_REG_INT_STATUS);
    clearOutputsAndHalt(F("FAIL: measurement start failed"));
  }
  Serial.println(F("PASS: measurement started"));

  uint32_t start = millis();
  while (!tmf8801.dataReady() && (millis() - start < 1000)) {
    delay(1);
  }
  if (!tmf8801.dataReady()) {
    clearOutputsAndHalt(F("FAIL: measurement timed out"));
  }
  Serial.println(F("PASS: measurement became ready"));

  tmf8801_result_t result;
  if (!tmf8801.readResult(&result)) {
    clearOutputsAndHalt(F("FAIL: result read failed"));
  }
  Serial.println(F("PASS: result read succeeded"));
  Serial.print(F("Distance: "));
  Serial.print(result.distance_mm);
  Serial.print(F(" mm, reliability: "));
  Serial.print(result.reliability);
  Serial.print(F(", reference hits: "));
  Serial.print(result.referenceHits);
  Serial.print(F(", object hits: "));
  Serial.println(result.objectHits);
  if (result.reliability == 0) {
    Serial.println(F("PASS: no-object result reported with zero reliability"));
  } else {
    Serial.println(F("PASS: valid distance measured"));
  }

  if (!tmf8801.stopMeasuring()) {
    clearOutputsAndHalt(F("FAIL: measurement stop failed"));
  }
  Serial.println(F("PASS: measurement stopped"));

  if (!tmf8801.sleep()) {
    clearOutputsAndHalt(F("FAIL: standby entry failed"));
  }
  Serial.println(F("PASS: standby entry succeeded"));

  if (!tmf8801.wakeup()) {
    clearOutputsAndHalt(F("FAIL: standby wake failed"));
  }
  Serial.println(F("PASS: standby wake succeeded"));
  Serial.println(F("ALL TESTS PASSED"));
}

void loop() { delay(1000); }
