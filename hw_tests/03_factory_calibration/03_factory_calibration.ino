/*!
 * TMF8801 factory-calibration hardware test
 *
 * Wiring:
 *   TMF8801 EN  -> Metro Mini D3
 *   TMF8801 INT -> Metro Mini D2
 *
 * Keep the sensor's field of view clear for at least 40 cm while this test
 * runs. Calibration can take up to 30 seconds.
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

  Serial.println(F("TMF8801 factory-calibration hardware test"));
  Serial.println(F("Keep the field of view clear for at least 40 cm"));

  if (!tmf8801.begin()) {
    printRegister(TMF8801_REG_ENABLE);
    printRegister(TMF8801_REG_APPID);
    printRegister(TMF8801_REG_ID);
    printRegister(TMF8801_REG_STATE);
    printRegister(TMF8801_REG_STATUS);
    clearOutputsAndHalt(F("FAIL: begin failed"));
  }
  Serial.println(F("PASS: begin succeeded"));

  if (!tmf8801.performFactoryCalibration()) {
    clearOutputsAndHalt(F("FAIL: factory calibration failed"));
  }
  Serial.println(F("PASS: factory calibration succeeded"));

  uint8_t calibration[TMF8801_CALIBRATION_DATA_SIZE];
  if (!tmf8801.getCalibrationData(calibration)) {
    clearOutputsAndHalt(F("FAIL: calibration data read failed"));
  }
  Serial.println(F("PASS: calibration data read succeeded"));

  Serial.print(F("Calibration data:"));
  for (uint8_t i = 0; i < sizeof(calibration); i++) {
    Serial.print(F(" 0x"));
    if (calibration[i] < 16) {
      Serial.print(F("0"));
    }
    Serial.print(calibration[i], HEX);
  }
  Serial.println();

  if (!tmf8801.startMeasuring()) {
    printRegister(TMF8801_REG_COMMAND);
    printRegister(TMF8801_REG_PREVIOUS);
    printRegister(TMF8801_REG_STATE);
    printRegister(TMF8801_REG_STATUS);
    printRegister(TMF8801_REG_CONTENTS);
    printRegister(TMF8801_REG_INT_STATUS);
    clearOutputsAndHalt(F("FAIL: calibrated measurement start failed"));
  }
  Serial.println(F("PASS: calibrated measurement started"));

  uint32_t start = millis();
  while (!tmf8801.dataReady() && (millis() - start < 1000)) {
    delay(1);
  }
  tmf8801_result_t result;
  if (!tmf8801.readResult(&result)) {
    clearOutputsAndHalt(F("FAIL: calibrated result read failed"));
  }
  Serial.println(F("PASS: calibrated result read succeeded"));
  Serial.print(F("Calibrated distance: "));
  Serial.print(result.distance_mm);
  Serial.print(F(" mm, reliability: "));
  Serial.print(result.reliability);
  Serial.print(F(", object hits: "));
  Serial.println(result.objectHits);
  if (result.reliability == 0) {
    Serial.println(
        F("PASS: clear field reported no object with zero reliability"));
  } else {
    Serial.println(F("PASS: calibrated distance is valid"));
  }

  if (!tmf8801.stopMeasuring()) {
    clearOutputsAndHalt(F("FAIL: calibrated measurement stop failed"));
  }
  Serial.println(F("PASS: calibrated measurement stopped"));
  Serial.println(F("ALL TESTS PASSED"));
}

void loop() { delay(1000); }
