/*!
 * @file tmf8801_fulltest.ino
 *
 * Demonstrate the complete public API of the Adafruit TMF8801 library.
 *
 * Factory calibration needs a clear field of view for at least 40 cm and can
 * take up to 30 seconds. The calibration is held in RAM; this example does not
 * write nonvolatile memory.
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

  Serial.println(F("Adafruit TMF8801 full API demonstration"));

  Serial.println(F("\n--- Begin and device information ---"));
  if (!tmf8801.begin(TMF8801_DEFAULT_ADDR, &Wire)) {
    haltWithMessage(F("Could not find a valid TMF8801 sensor, check wiring!"));
  }
  Serial.println(F("TMF8801 found"));

  uint8_t chipID = tmf8801.getChipID();
  Serial.print(F("Chip ID: 0x"));
  printHexByte(chipID);
  Serial.println();

  Serial.print(F("Revision ID: 0x"));
  printHexByte(tmf8801.getRevisionID());
  Serial.println();

  Serial.print(F("Application status: 0x"));
  printHexByte(tmf8801.getStatus());
  Serial.println();

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

  uint16_t serialNumber = 0;
  if (!tmf8801.readSerialNumber(&serialNumber)) {
    haltWithMessage(F("Could not read the serial number"));
  }
  Serial.print(F("Serial number: 0x"));
  printHexWord(serialNumber);
  Serial.println();

  Serial.println(F("\n--- Configuration writes and reads ---"));
  Serial.println(F("Iterations are a 16-bit value from 0 to 65,535."));
  Serial.println(F("Each count is 1,000 iterations; 900 is the default."));
  Serial.println(F("More iterations can improve signal but take longer."));
  tmf8801.setIterations(900);
  Serial.print(F("Current iterations: "));
  Serial.print(tmf8801.getIterations());
  Serial.println(F(",000"));

  Serial.println();
  Serial.println(F("Repetition period 0 selects one-shot mode."));
  Serial.println(F("Use 1 to 255 ms for continuous measurements."));
  Serial.println(
      F("If a reading takes longer, the next one starts as soon as it can."));
  tmf8801.setRepetitionPeriod(50);
  Serial.print(F("Current repetition period: "));
  Serial.print(tmf8801.getRepetitionPeriod());
  Serial.println(F(" ms"));

  Serial.println();
  Serial.println(F("Noise threshold is a 6-bit value from 0 to 63."));
  Serial.println(F("Use 0 to keep the sensor's default detection threshold."));
  tmf8801.setNoiseThreshold(5);
  Serial.print(F("Current noise threshold: "));
  Serial.println(tmf8801.getNoiseThreshold());

  Serial.println();
  Serial.println(F("GPIO modes: 0 disabled, 1 active-low input,"));
  Serial.println(F("2 active-high input, 3 VCSEL output, 4 low, 5 high."));
  tmf8801.setGPIOMode(0, TMF8801_GPIO_OUTPUT_LOW);
  tmf8801.setGPIOMode(1, TMF8801_GPIO_OUTPUT_HIGH);
  Serial.print(F("GPIO 0 mode: "));
  printGPIOMode(tmf8801.getGPIOMode(0));
  Serial.println();
  Serial.print(F("GPIO 1 mode: "));
  printGPIOMode(tmf8801.getGPIOMode(1));
  Serial.println();

  Serial.println(F("\n--- Continuous measurement and full result ---"));
  if (!tmf8801.startMeasuring(true)) {
    haltWithMessage(F("Could not start continuous measurements"));
  }
  Serial.println(F("Continuous measurement started"));

  if (!waitForData(1000)) {
    haltWithMessage(F("Continuous measurement timed out"));
  }

  tmf8801_result_t result;
  if (!tmf8801.readResult(&result)) {
    haltWithMessage(F("Could not read the complete result"));
  }
  printResult(&result);

  uint8_t algorithmState[TMF8801_ALGORITHM_STATE_SIZE];
  if (!tmf8801.getAlgorithmState(algorithmState)) {
    haltWithMessage(F("Could not read the algorithm state"));
  }
  Serial.print(F("Algorithm state:"));
  printBytes(algorithmState, sizeof(algorithmState));

  if (!tmf8801.stopMeasuring()) {
    haltWithMessage(F("Could not stop continuous measurements"));
  }
  Serial.println(F("Continuous measurement stopped"));

  Serial.println(F("\n--- Single-shot distance helper ---"));
  tmf8801.setNoiseThreshold(0);
  tmf8801.setGPIOMode(0, TMF8801_GPIO_DISABLED);
  tmf8801.setGPIOMode(1, TMF8801_GPIO_DISABLED);
  if (!tmf8801.startMeasuring(false)) {
    haltWithMessage(F("Could not start a single-shot measurement"));
  }
  Serial.println(F("Single-shot measurement started"));

  if (!waitForData(1000)) {
    haltWithMessage(F("Single-shot measurement timed out"));
  }
  int16_t distance = tmf8801.readDistance();
  if (distance < 0) {
    Serial.println(F("readDistance(): no reliable object detected"));
  } else {
    Serial.print(F("readDistance(): "));
    Serial.print(distance);
    Serial.println(F(" mm"));
  }

  Serial.println(F("\n--- Factory calibration and data replay ---"));
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

  if (!tmf8801.setCalibrationData(calibration)) {
    haltWithMessage(F("setCalibrationData() returned false"));
  }
  Serial.println(F("setCalibrationData() returned true"));
  tmf8801.enableCalibration(true);
  Serial.println(F("Calibration replay enabled"));

  if (!tmf8801.startMeasuring(false)) {
    haltWithMessage(F("Could not start a calibrated measurement"));
  }
  if (!waitForData(1000)) {
    haltWithMessage(F("Calibrated measurement timed out"));
  }
  if (!tmf8801.readResult(&result)) {
    haltWithMessage(F("Could not read the calibrated result"));
  }
  Serial.println(F("Calibrated result:"));
  printResult(&result);

  if (!tmf8801.getAlgorithmState(algorithmState)) {
    haltWithMessage(F("Could not read the calibrated algorithm state"));
  }
  Serial.print(F("Calibrated algorithm state:"));
  printBytes(algorithmState, sizeof(algorithmState));

  if (!tmf8801.setAlgorithmState(algorithmState)) {
    haltWithMessage(F("setAlgorithmState() returned false"));
  }
  Serial.println(F("setAlgorithmState() returned true"));
  tmf8801.enableAlgorithmState(true);
  Serial.println(F("Algorithm-state replay enabled"));

  if (!tmf8801.startMeasuring(false)) {
    haltWithMessage(F("Could not start an algorithm-state replay measurement"));
  }
  if (!waitForData(1000)) {
    haltWithMessage(F("Algorithm-state replay measurement timed out"));
  }
  if (!tmf8801.readResult(&result)) {
    haltWithMessage(F("Could not read the algorithm-state replay result"));
  }
  Serial.println(F("Calibrated result with algorithm-state replay:"));
  printResult(&result);

  tmf8801.enableAlgorithmState(false);
  tmf8801.enableCalibration(false);
  Serial.println(F("Calibration and algorithm-state replay disabled"));

  Serial.println(F("\n--- Standby, wake, and reset ---"));
  if (!tmf8801.sleep()) {
    haltWithMessage(F("Could not enter standby"));
  }
  Serial.println(F("Sensor entered standby"));
  delay(10);

  if (!tmf8801.wakeup()) {
    haltWithMessage(F("Could not wake from standby"));
  }
  Serial.println(F("Sensor woke from standby"));

  if (!tmf8801.reset()) {
    haltWithMessage(F("Could not reset and reload the measurement firmware"));
  }
  Serial.println(F("Sensor reset and measurement firmware reloaded"));

  Serial.print(F("Chip ID after reset: 0x"));
  printHexByte(tmf8801.getChipID());
  Serial.println();
  Serial.print(F("Status after reset: 0x"));
  printHexByte(tmf8801.getStatus());
  Serial.println();

  Serial.println(F("\nFULL API DEMONSTRATION COMPLETE"));
}

void loop() { delay(1000); }

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

void printHexWord(uint16_t value) {
  printHexByte(highByte(value));
  printHexByte(lowByte(value));
}

void printBytes(const uint8_t *data, uint8_t length) {
  for (uint8_t i = 0; i < length; i++) {
    Serial.print(F(" 0x"));
    printHexByte(data[i]);
  }
  Serial.println();
}

void printGPIOMode(tmf8801_gpio_mode_t mode) {
  Serial.print((uint8_t)mode);
  Serial.print(F(" ("));
  switch (mode) {
  case TMF8801_GPIO_DISABLED:
    Serial.print(F("disabled"));
    break;
  case TMF8801_GPIO_INPUT_ACTIVE_LOW:
    Serial.print(F("input active low"));
    break;
  case TMF8801_GPIO_INPUT_ACTIVE_HIGH:
    Serial.print(F("input active high"));
    break;
  case TMF8801_GPIO_OUTPUT_VCSEL:
    Serial.print(F("VCSEL output"));
    break;
  case TMF8801_GPIO_OUTPUT_LOW:
    Serial.print(F("output low"));
    break;
  case TMF8801_GPIO_OUTPUT_HIGH:
    Serial.print(F("output high"));
    break;
  default:
    Serial.print(F("unknown"));
    break;
  }
  Serial.print(F(")"));
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

void printResult(const tmf8801_result_t *result) {
  Serial.print(F("Result number: "));
  Serial.println(result->number);
  Serial.print(F("Distance: "));
  Serial.print(result->distance_mm);
  Serial.print(F(" mm   Reliability: "));
  printReliability(result->reliability);
  Serial.print(F("   Status: "));
  printMeasurementStatus(result->status);
  Serial.println();
  Serial.print(F("System clock: "));
  Serial.println(result->systemClock);
  Serial.print(F("Reference hits: "));
  Serial.println(result->referenceHits);
  Serial.print(F("Object hits: "));
  Serial.println(result->objectHits);
}
