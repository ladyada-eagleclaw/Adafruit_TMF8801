/*!
 * @file tmf8801_factorycalibration.ino
 *
 * Factory calibrate the TMF8801, then use the calibration and algorithm state
 * for one-shot measurements.
 *
 * Calibration needs a clear field of view for at least 40 cm and can take up
 * to 30 seconds. The calibration is held in RAM and is lost when the sensor is
 * reset or loses power. This example does not write nonvolatile memory.
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

  Serial.println(F("Adafruit TMF8801 factory calibration"));

  if (!tmf8801.begin()) {
    haltWithMessage(F("Could not find a valid TMF8801 sensor, check wiring!"));
  }
  Serial.println(F("TMF8801 found"));

  Serial.println(F("Clear all objects within 40 cm of the sensor."));
  for (uint8_t seconds = 5; seconds > 0; seconds--) {
    Serial.print(F("Calibration starts in "));
    Serial.print(seconds);
    Serial.println(F("..."));
    delay(1000);
  }

  if (!tmf8801.performFactoryCalibration()) {
    haltWithMessage(F("Factory calibration did not complete"));
  }
  Serial.println(F("Factory calibration completed"));

  uint8_t calibration[TMF8801_CALIBRATION_DATA_SIZE];
  if (!tmf8801.getCalibrationData(calibration)) {
    haltWithMessage(F("Could not read the factory calibration data"));
  }
  Serial.print(F("Factory calibration:"));
  printBytes(calibration, sizeof(calibration));

  // Save these bytes outside the sensor if calibration must survive a reset or
  // power loss. setCalibrationData() restores previously saved calibration.
  if (!tmf8801.setCalibrationData(calibration)) {
    haltWithMessage(F("Could not set the factory calibration data"));
  }
  tmf8801.enableCalibration(true);
  Serial.println(F("Calibration replay enabled"));
}

void loop() {
  if (!tmf8801.startMeasuring(false)) {
    haltWithMessage(F("Could not start a calibrated measurement"));
  }
  if (!waitForData(1000)) {
    haltWithMessage(F("Calibrated measurement timed out"));
  }

  int16_t distance = tmf8801.readDistance();
  if (distance < 0) {
    Serial.println(F("No reliable object detected"));
  } else {
    Serial.print(F("Distance: "));
    Serial.print(distance);
    Serial.println(F(" mm"));
  }

  uint8_t algorithmState[TMF8801_ALGORITHM_STATE_SIZE];
  if (!tmf8801.getAlgorithmState(algorithmState)) {
    haltWithMessage(F("Could not read the algorithm state"));
  }
  Serial.print(F("Algorithm state:"));
  printBytes(algorithmState, sizeof(algorithmState));

  // Replay the latest algorithm state with the calibration on the next reading.
  if (!tmf8801.setAlgorithmState(algorithmState)) {
    haltWithMessage(F("Could not set the algorithm state"));
  }
  tmf8801.enableAlgorithmState(true);
  Serial.println();
}

void haltWithMessage(const __FlashStringHelper *message) {
  Serial.print(F("Stopped: "));
  Serial.println(message);
  while (1) {
    delay(10);
  }
}

bool waitForData(uint16_t timeoutMs) {
  uint32_t start = millis();
  while (!tmf8801.dataReady() && (millis() - start < timeoutMs)) {
    delay(1);
  }
  return tmf8801.dataReady();
}

void printHexByte(uint8_t value) {
  if (value < 16) {
    Serial.print(F("0"));
  }
  Serial.print(value, HEX);
}

void printBytes(const uint8_t *data, uint8_t length) {
  for (uint8_t i = 0; i < length; i++) {
    Serial.print(F(" 0x"));
    printHexByte(data[i]);
  }
  Serial.println();
}
