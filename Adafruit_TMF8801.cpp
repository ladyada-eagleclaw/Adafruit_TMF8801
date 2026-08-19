/*!
 * @file Adafruit_TMF8801.cpp
 *
 * @mainpage Adafruit TMF8801 Time-of-Flight Sensor Library
 *
 * @section intro_sec Introduction
 *
 * Arduino driver for the ams OSRAM TMF8801 1D direct Time-of-Flight sensor.
 *
 * @section author Author
 *
 * Written by Limor Fried/Ladyada for Adafruit Industries.
 *
 * @section license License
 *
 * MIT license, all text above must be included in any redistribution.
 */

#include "Adafruit_TMF8801.h"

#include "Adafruit_TMF8801_firmware.h"

/*!
 * @brief Construct a new TMF8801 sensor object.
 */
Adafruit_TMF8801::Adafruit_TMF8801() {
  _i2c_dev = nullptr;
  _iterations = 900;
  _repetitionPeriod = 33;
  _noiseThreshold = 0;
  _gpio0Mode = TMF8801_GPIO_DISABLED;
  _gpio1Mode = TMF8801_GPIO_DISABLED;
  _useCalibration = false;
  _hasCalibration = false;
  _useAlgorithmState = false;
  _hasAlgorithmState = false;
  memset(_calibrationData, 0, sizeof(_calibrationData));
  memset(_algorithmState, 0, sizeof(_algorithmState));
}

/*!
 * @brief Destroy the TMF8801 sensor object and release its I2C device.
 */
Adafruit_TMF8801::~Adafruit_TMF8801() {
  if (_i2c_dev) {
    delete _i2c_dev;
  }
}

/*!
 * @brief Initialize the sensor, load its firmware, and verify the chip ID.
 *
 * @param addr The sensor's I2C address.
 * @param wire The I2C interface to use.
 * @return True if initialization succeeded, otherwise false.
 */
bool Adafruit_TMF8801::begin(uint8_t addr, TwoWire* wire) {
  if (_i2c_dev) {
    delete _i2c_dev;
  }
  _i2c_dev = new Adafruit_I2CDevice(addr, wire);

  if (!_i2c_dev->begin()) {
    return false;
  }
  // Reset into a known state so begin() also recovers from an application
  // error left by a previous sketch or interrupted command.
  if (!reset()) {
    return false;
  }
  return getChipID() == TMF8801_CHIP_ID;
}

/*!
 * @brief Reset the sensor and reload its RAM measurement firmware.
 *
 * @return True if the measurement application restarted, otherwise false.
 */
bool Adafruit_TMF8801::reset() {
  if (!_i2c_dev) {
    return false;
  }

  Adafruit_BusIO_Register enable =
      Adafruit_BusIO_Register(_i2c_dev, TMF8801_REG_ENABLE);
  // Reset into the bootloader. The measurement application is held in RAM,
  // so it must be uploaded again after every reset or power cycle.
  if (!enable.write(TMF8801_ENABLE_RESET | TMF8801_ENABLE_POWER_ON)) {
    return false;
  }
  delay(2);
  if (!enable.write(TMF8801_ENABLE_POWER_ON) || !waitForCpuReady(100) ||
      !waitForBootloader(100)) {
    return false;
  }
  if (!uploadFirmware()) {
    return false;
  }
  return startApp();
}

/*!
 * @brief Start distance measurements using the current configuration.
 *
 * @param continuous True for continuous measurements, or false for one-shot.
 * @return True if the measurement command was accepted, otherwise false.
 */
bool Adafruit_TMF8801::startMeasuring(bool continuous) {
  if (!_i2c_dev) {
    return false;
  }

  bool useCalibration = _useCalibration && _hasCalibration;
  bool useState = _useAlgorithmState && _hasAlgorithmState && useCalibration;
  uint8_t dataFlags = 0;

  if (useCalibration) {
    uint8_t reg = TMF8801_REG_RESULT_NUMBER;
    if (!_i2c_dev->write(_calibrationData, sizeof(_calibrationData), true, &reg,
                         1)) {
      return false;
    }
    dataFlags |= TMF8801_DATA_FACTORY_CALIBRATION;
  }

  if (useState) {
    uint8_t replayState[TMF8801_ALGORITHM_STATE_SIZE];
    memcpy(replayState, _algorithmState, sizeof(replayState));
    memset(&replayState[3], 0, sizeof(replayState) - 3);
    uint8_t reg = TMF8801_REG_RESULT_NUMBER + TMF8801_CALIBRATION_DATA_SIZE;
    if (!_i2c_dev->write(replayState, sizeof(replayState), true, &reg, 1)) {
      return false;
    }
    dataFlags |= TMF8801_DATA_ALGORITHM_STATE;
  }

  uint8_t command[11] = {0};
  command[2] = dataFlags;
  command[3] = TMF8801_ALGORITHM_DEFAULT;
  command[4] =
      (_gpio0Mode & TMF8801_GPIO_MODE_MASK) |
      ((_gpio1Mode & TMF8801_GPIO_MODE_MASK) << TMF8801_GPIO1_MODE_SHIFT);
  command[5] = 0;
  command[6] = _noiseThreshold;
  command[7] = continuous ? _repetitionPeriod : 0;
  command[8] = lowByte(_iterations);
  command[9] = highByte(_iterations);
  command[10] = dataFlags ? TMF8801_CMD_MEASURE_WITH_CAL : TMF8801_CMD_MEASURE;

  uint8_t reg = TMF8801_REG_CMD_DATA9;
  if (!_i2c_dev->write(command, sizeof(command), true, &reg, 1)) {
    return false;
  }
  return waitForCommand(command[10], 20);
}

/*!
 * @brief Stop measurements and wait for the sensor to become idle.
 *
 * @return True if measurement stopped successfully, otherwise false.
 */
bool Adafruit_TMF8801::stopMeasuring() {
  if (!_i2c_dev) {
    return false;
  }

  Adafruit_BusIO_Register command =
      Adafruit_BusIO_Register(_i2c_dev, TMF8801_REG_COMMAND);
  if (!command.write(TMF8801_CMD_STOP)) {
    return false;
  }
  if (!waitForCommand(TMF8801_CMD_STOP, 250)) {
    return false;
  }
  return waitForIdle(250);
}

/*!
 * @brief Check whether a new measurement result is ready.
 *
 * @return True when a result is ready to read, otherwise false.
 */
bool Adafruit_TMF8801::dataReady() {
  if (!_i2c_dev) {
    return false;
  }
  Adafruit_BusIO_Register interruptStatus =
      Adafruit_BusIO_Register(_i2c_dev, TMF8801_REG_INT_STATUS);
  return (interruptStatus.read() & TMF8801_INT_RESULT) != 0;
}

/*!
 * @brief Read the measured distance as a convenience operation.
 *
 * @return Distance in millimeters, or -1 if no reliable result was available.
 */
int16_t Adafruit_TMF8801::readDistance() {
  tmf8801_result_t result;
  if (!readResult(&result)) {
    return -1;
  }
  if (result.reliability == 0) {
    return -1;
  }
  return (int16_t)result.distance_mm;
}

/*!
 * @brief Read the complete measurement result.
 *
 * @param result Pointer to the result structure to fill.
 * @return True if a valid result was read, otherwise false.
 */
bool Adafruit_TMF8801::readResult(tmf8801_result_t* result) {
  if (!_i2c_dev || !result || !dataReady()) {
    return false;
  }

  // Read from STATUS through the complete result in one transaction. The
  // sensor uses this block read to update the result and system clock together.
  uint8_t frame[TMF8801_RESULT_FRAME_SIZE];
  uint8_t reg = TMF8801_REG_STATUS;
  if (!_i2c_dev->write_then_read(&reg, 1, frame, sizeof(frame))) {
    return false;
  }
  if (frame[TMF8801_RESULT_CONTENTS_OFFSET] != TMF8801_CONTENT_RESULT) {
    return false;
  }

  const uint8_t* data = &frame[TMF8801_RESULT_HEADER_SIZE];

  result->number = data[0];
  result->reliability = data[1] & TMF8801_RESULT_RELIABILITY_MASK;
  result->status =
      (tmf8801_measurement_status_t)(data[1] >> TMF8801_RESULT_STATUS_SHIFT);
  result->distance_mm = data[2] | ((uint16_t)data[3] << 8);
  result->systemClock = data[4] | ((uint32_t)data[5] << 8) |
                        ((uint32_t)data[6] << 16) | ((uint32_t)data[7] << 24);
  memcpy(_algorithmState, &data[8], TMF8801_ALGORITHM_STATE_SIZE);
  _hasAlgorithmState = true;
  result->referenceHits = data[19] | ((uint32_t)data[20] << 8) |
                          ((uint32_t)data[21] << 16) |
                          ((uint32_t)data[22] << 24);
  result->objectHits = data[23] | ((uint32_t)data[24] << 8) |
                       ((uint32_t)data[25] << 16) | ((uint32_t)data[26] << 24);

  return clearInterrupt(TMF8801_INT_RESULT);
}

/*!
 * @brief Set the number of measurement iterations.
 *
 * @param kiloIterations Number of iterations in thousands.
 */
void Adafruit_TMF8801::setIterations(uint16_t kiloIterations) {
  _iterations = kiloIterations;
}

/*!
 * @brief Get the configured number of measurement iterations.
 *
 * @return Number of iterations in thousands.
 */
uint16_t Adafruit_TMF8801::getIterations() {
  return _iterations;
}

/*!
 * @brief Set the requested interval between continuous results.
 *
 * @param periodMs Result interval in milliseconds, from 1 to 255.
 */
void Adafruit_TMF8801::setRepetitionPeriod(uint8_t periodMs) {
  _repetitionPeriod = periodMs;
}

/*!
 * @brief Get the requested interval between continuous results.
 *
 * @return Result interval in milliseconds.
 */
uint8_t Adafruit_TMF8801::getRepetitionPeriod() {
  return _repetitionPeriod;
}

/*!
 * @brief Set the object-detection noise threshold.
 *
 * @param threshold Noise threshold from 0 to 255. Zero uses the default.
 */
void Adafruit_TMF8801::setNoiseThreshold(uint8_t threshold) {
  _noiseThreshold = threshold;
}

/*!
 * @brief Get the configured object-detection noise threshold.
 *
 * @return The configured noise threshold.
 */
uint8_t Adafruit_TMF8801::getNoiseThreshold() {
  return _noiseThreshold;
}

/*!
 * @brief Configure one of the sensor's GPIO pins for measurement.
 *
 * @param gpio GPIO number, either 0 or 1.
 * @param mode Function to assign to the selected GPIO pin.
 */
void Adafruit_TMF8801::setGPIOMode(uint8_t gpio, tmf8801_gpio_mode_t mode) {
  if (gpio == 0) {
    _gpio0Mode = (uint8_t)mode;
  } else if (gpio == 1) {
    _gpio1Mode = (uint8_t)mode;
  }
}

/*!
 * @brief Get the configured mode for one of the sensor's GPIO pins.
 *
 * @param gpio GPIO number, either 0 or 1.
 * @return The selected GPIO pin's configured mode.
 */
tmf8801_gpio_mode_t Adafruit_TMF8801::getGPIOMode(uint8_t gpio) {
  if (gpio == 0) {
    return (tmf8801_gpio_mode_t)_gpio0Mode;
  }
  return (tmf8801_gpio_mode_t)_gpio1Mode;
}

/*!
 * @brief Run factory calibration and save the resulting data.
 *
 * @return True if calibration completed and data was read, otherwise false.
 */
bool Adafruit_TMF8801::performFactoryCalibration() {
  if (!_i2c_dev) {
    return false;
  }

  uint8_t command[11] = {0};
  command[10] = TMF8801_CMD_FACTORY_CALIB;
  uint8_t reg = TMF8801_REG_CMD_DATA9;
  if (!_i2c_dev->write(command, sizeof(command), true, &reg, 1)) {
    return false;
  }
  if (!waitForCommand(TMF8801_CMD_FACTORY_CALIB, 20)) {
    return false;
  }

  uint32_t start = millis();
  while (!dataReady() && (millis() - start < 30000)) {
    delay(10);
  }
  if (!dataReady()) {
    return false;
  }

  uint8_t contents = 0;
  reg = TMF8801_REG_CONTENTS;
  if (!_i2c_dev->write_then_read(&reg, 1, &contents, 1) ||
      contents != TMF8801_CONTENT_CALIBRATION) {
    return false;
  }

  reg = TMF8801_REG_RESULT_NUMBER;
  if (!_i2c_dev->write_then_read(&reg, 1, _calibrationData,
                                 sizeof(_calibrationData))) {
    return false;
  }
  _hasCalibration = true;
  _useCalibration = true;
  return clearInterrupt(TMF8801_INT_RESULT);
}

/*!
 * @brief Copy the saved factory calibration data into a caller buffer.
 *
 * @param data Buffer that receives the calibration data.
 * @param len Size of the destination buffer in bytes.
 * @return True if saved data was available and copied, otherwise false.
 */
bool Adafruit_TMF8801::getCalibrationData(uint8_t* data, uint8_t len) {
  if (!data || len < TMF8801_CALIBRATION_DATA_SIZE || !_hasCalibration) {
    return false;
  }
  memcpy(data, _calibrationData, TMF8801_CALIBRATION_DATA_SIZE);
  return true;
}

/*!
 * @brief Store factory calibration data for use during measurement.
 *
 * @param data Buffer containing the calibration data.
 * @param len Size of the source buffer in bytes.
 * @return True if the calibration data was stored, otherwise false.
 */
bool Adafruit_TMF8801::setCalibrationData(const uint8_t* data, uint8_t len) {
  if (!data || len < TMF8801_CALIBRATION_DATA_SIZE) {
    return false;
  }
  memcpy(_calibrationData, data, TMF8801_CALIBRATION_DATA_SIZE);
  _hasCalibration = true;
  return true;
}

/*!
 * @brief Enable or disable use of saved factory calibration data.
 *
 * @param enable True to use saved calibration data, or false to ignore it.
 */
void Adafruit_TMF8801::enableCalibration(bool enable) {
  _useCalibration = enable;
}

/*!
 * @brief Copy the most recently captured algorithm state to a caller buffer.
 *
 * @param data Buffer that receives the algorithm state.
 * @param len Size of the destination buffer in bytes.
 * @return True if saved state was available and copied, otherwise false.
 */
bool Adafruit_TMF8801::getAlgorithmState(uint8_t* data, uint8_t len) {
  if (!data || len < TMF8801_ALGORITHM_STATE_SIZE || !_hasAlgorithmState) {
    return false;
  }
  memcpy(data, _algorithmState, TMF8801_ALGORITHM_STATE_SIZE);
  return true;
}

/*!
 * @brief Store algorithm state for use during measurement.
 *
 * @param data Buffer containing the algorithm state.
 * @param len Size of the source buffer in bytes.
 * @return True if the algorithm state was stored, otherwise false.
 */
bool Adafruit_TMF8801::setAlgorithmState(const uint8_t* data, uint8_t len) {
  if (!data || len < TMF8801_ALGORITHM_STATE_SIZE) {
    return false;
  }
  memcpy(_algorithmState, data, TMF8801_ALGORITHM_STATE_SIZE);
  _hasAlgorithmState = true;
  return true;
}

/*!
 * @brief Enable or disable use of saved algorithm state.
 *
 * @param enable True to use saved state with calibration, or false to ignore.
 */
void Adafruit_TMF8801::enableAlgorithmState(bool enable) {
  _useAlgorithmState = enable;
}

/*!
 * @brief Read the sensor chip ID.
 *
 * @return The chip ID, or 0 if the sensor has not been initialized.
 */
uint8_t Adafruit_TMF8801::getChipID() {
  if (!_i2c_dev) {
    return 0;
  }
  Adafruit_BusIO_Register chipId =
      Adafruit_BusIO_Register(_i2c_dev, TMF8801_REG_ID);
  return chipId.read();
}

/*!
 * @brief Read the sensor hardware revision ID.
 *
 * @return The revision ID, or 0 if the sensor has not been initialized.
 */
uint8_t Adafruit_TMF8801::getRevisionID() {
  if (!_i2c_dev) {
    return 0;
  }
  Adafruit_BusIO_Register revision =
      Adafruit_BusIO_Register(_i2c_dev, TMF8801_REG_REVID);
  return revision.read();
}

/*!
 * @brief Read the measurement application's status register.
 *
 * @return The status, or 0 if the sensor has not been initialized.
 */
uint8_t Adafruit_TMF8801::getStatus() {
  if (!_i2c_dev) {
    return 0;
  }
  Adafruit_BusIO_Register status =
      Adafruit_BusIO_Register(_i2c_dev, TMF8801_REG_STATUS);
  return status.read();
}

/*!
 * @brief Read the measurement application firmware version.
 *
 * @param major Pointer that receives the major version.
 * @param minor Pointer that receives the minor version.
 * @param patch Pointer that receives the patch version.
 */
void Adafruit_TMF8801::getVersion(uint8_t* major, uint8_t* minor,
                                  uint8_t* patch) {
  if (!_i2c_dev || !major || !minor || !patch) {
    return;
  }

  Adafruit_BusIO_Register majorReg =
      Adafruit_BusIO_Register(_i2c_dev, TMF8801_REG_APPREV_MAJOR);
  Adafruit_BusIO_Register minorReg =
      Adafruit_BusIO_Register(_i2c_dev, TMF8801_REG_APPREV_MINOR);
  Adafruit_BusIO_Register patchReg =
      Adafruit_BusIO_Register(_i2c_dev, TMF8801_REG_APPREV_PATCH);
  *major = majorReg.read();
  *minor = minorReg.read();
  *patch = patchReg.read();
}

/*!
 * @brief Read the sensor's serial number.
 *
 * @param serialNumber Pointer that receives the 16-bit serial number.
 * @return True if the serial number was read, otherwise false.
 */
bool Adafruit_TMF8801::readSerialNumber(uint16_t* serialNumber) {
  if (!_i2c_dev || !serialNumber) {
    return false;
  }

  Adafruit_BusIO_Register command =
      Adafruit_BusIO_Register(_i2c_dev, TMF8801_REG_COMMAND);
  if (!command.write(TMF8801_CMD_READ_SERIAL)) {
    return false;
  }
  if (!waitForCommand(TMF8801_CMD_READ_SERIAL, 100)) {
    return false;
  }

  uint32_t start = millis();
  uint8_t contents = 0;
  uint8_t reg = TMF8801_REG_CONTENTS;
  while ((millis() - start) < 100) {
    if (!_i2c_dev->write_then_read(&reg, 1, &contents, 1)) {
      return false;
    }
    if (contents == TMF8801_CMD_READ_SERIAL) {
      break;
    }
    delay(1);
  }
  if (contents != TMF8801_CMD_READ_SERIAL) {
    return false;
  }

  uint8_t serial[2];
  reg = TMF8801_REG_RESULT_NUMBER + 8;
  if (!_i2c_dev->write_then_read(&reg, 1, serial, sizeof(serial))) {
    return false;
  }
  *serialNumber = serial[0] | ((uint16_t)serial[1] << 8);
  return true;
}

/*!
 * @brief Put the sensor into its low-power state.
 *
 * @return True if the power control bit was cleared, otherwise false.
 */
bool Adafruit_TMF8801::sleep() {
  if (!_i2c_dev) {
    return false;
  }
  Adafruit_BusIO_Register enable =
      Adafruit_BusIO_Register(_i2c_dev, TMF8801_REG_ENABLE);
  Adafruit_BusIO_RegisterBits powerOn =
      Adafruit_BusIO_RegisterBits(&enable, 1, TMF8801_ENABLE_POWER_ON_BIT);
  return powerOn.write(0);
}

/*!
 * @brief Wake the sensor from its low-power state.
 *
 * @return True if the sensor powered up and became ready, otherwise false.
 */
bool Adafruit_TMF8801::wakeup() {
  if (!_i2c_dev) {
    return false;
  }
  Adafruit_BusIO_Register enable =
      Adafruit_BusIO_Register(_i2c_dev, TMF8801_REG_ENABLE);
  Adafruit_BusIO_RegisterBits powerOn =
      Adafruit_BusIO_RegisterBits(&enable, 1, TMF8801_ENABLE_POWER_ON_BIT);
  if (!powerOn.write(1)) {
    return false;
  }
  return waitForCpuReady(100);
}

bool Adafruit_TMF8801::startApp() {
  Adafruit_BusIO_Register enable =
      Adafruit_BusIO_Register(_i2c_dev, TMF8801_REG_ENABLE);
  Adafruit_BusIO_RegisterBits powerOn =
      Adafruit_BusIO_RegisterBits(&enable, 1, TMF8801_ENABLE_POWER_ON_BIT);
  if (!powerOn.write(1)) {
    return false;
  }
  if (!waitForCpuReady(100)) {
    return false;
  }

  Adafruit_BusIO_Register appId =
      Adafruit_BusIO_Register(_i2c_dev, TMF8801_REG_APPID);
  if (appId.read() != TMF8801_APP_MEASUREMENT) {
    Adafruit_BusIO_Register appRequest =
        Adafruit_BusIO_Register(_i2c_dev, TMF8801_REG_APPREQID);
    if (!appRequest.write(TMF8801_APP_MEASUREMENT)) {
      return false;
    }
    if (!waitForApp(200)) {
      return false;
    }
  }

  if (!clearInterrupt(TMF8801_INT_RESULT | TMF8801_INT_ERROR)) {
    return false;
  }
  Adafruit_BusIO_Register interruptEnable =
      Adafruit_BusIO_Register(_i2c_dev, TMF8801_REG_INT_ENABLE);
  return interruptEnable.write(TMF8801_INT_RESULT | TMF8801_INT_ERROR);
}

bool Adafruit_TMF8801::uploadFirmware() {
  const uint8_t salt = TMF8801_BL_DEFAULT_SALT;
  const uint8_t address[] = {lowByte(TMF8801_BL_RAM_ADDRESS),
                             highByte(TMF8801_BL_RAM_ADDRESS)};
  uint8_t chunk[TMF8801_BL_CHUNK_SIZE];

  if (!sendBootloaderCommand(TMF8801_BL_UPLOAD_INIT, &salt, 1) ||
      !sendBootloaderCommand(TMF8801_BL_ADDRESS_RAM, address,
                             sizeof(address))) {
    return false;
  }

  for (uint16_t offset = 0; offset < TMF8801_FIRMWARE_SIZE;
       offset += TMF8801_BL_CHUNK_SIZE) {
    uint8_t chunkSize = min((uint16_t)TMF8801_BL_CHUNK_SIZE,
                            (uint16_t)(TMF8801_FIRMWARE_SIZE - offset));
    for (uint8_t i = 0; i < chunkSize; i++) {
      chunk[i] = pgm_read_byte(&tmf8801_firmware[offset + i]);
    }
    if (!sendBootloaderCommand(TMF8801_BL_WRITE_RAM, chunk, chunkSize)) {
      return false;
    }
  }

  // RAM remap restarts the CPU, so there is no bootloader response to read.
  uint8_t packet[] = {TMF8801_BL_RAM_REMAP, TMF8801_BL_NO_DATA,
                      (uint8_t)~TMF8801_BL_RAM_REMAP};
  uint8_t reg = TMF8801_REG_BOOTLOADER;
  if (!_i2c_dev->write(packet, sizeof(packet), true, &reg, 1)) {
    return false;
  }
  return waitForCpuReady(200) && waitForApp(200);
}

bool Adafruit_TMF8801::sendBootloaderCommand(uint8_t command,
                                             const uint8_t* data, uint8_t len) {
  if (len > TMF8801_BL_CHUNK_SIZE || (len > 0 && !data)) {
    return false;
  }
  uint8_t packet[TMF8801_BL_CHUNK_SIZE + 3];
  uint8_t checksum = command + len;
  packet[0] = command;
  packet[1] = len;
  for (uint8_t i = 0; i < len; i++) {
    packet[i + 2] = data[i];
    checksum += data[i];
  }
  packet[len + 2] = ~checksum;

  uint8_t reg = TMF8801_REG_BOOTLOADER;
  if (!_i2c_dev->write(packet, len + 3, true, &reg, 1)) {
    return false;
  }

  uint32_t start = millis();
  while ((millis() - start) < 20) {
    uint8_t response[3];
    if (!_i2c_dev->write_then_read(&reg, 1, response, sizeof(response))) {
      return false;
    }
    if (response[0] >= TMF8801_BL_BUSY) {
      delay(1);
      continue;
    }
    return response[0] == TMF8801_BL_READY &&
           response[1] == TMF8801_BL_NO_DATA &&
           (uint8_t)(response[0] + response[1] + response[2]) ==
               TMF8801_BL_VALID_CHECKSUM;
  }
  return false;
}

bool Adafruit_TMF8801::waitForBootloader(uint16_t timeoutMs) {
  Adafruit_BusIO_Register appId =
      Adafruit_BusIO_Register(_i2c_dev, TMF8801_REG_APPID);
  uint32_t start = millis();
  while ((millis() - start) < timeoutMs) {
    if (appId.read() == TMF8801_APP_BOOTLOADER) {
      return true;
    }
    delay(1);
  }
  return false;
}

bool Adafruit_TMF8801::waitForCpuReady(uint16_t timeoutMs) {
  Adafruit_BusIO_Register enable =
      Adafruit_BusIO_Register(_i2c_dev, TMF8801_REG_ENABLE);
  uint32_t start = millis();
  while ((millis() - start) < timeoutMs) {
    if (enable.read() == (TMF8801_ENABLE_CPU_READY | TMF8801_ENABLE_POWER_ON)) {
      return true;
    }
    delay(1);
  }
  return false;
}

bool Adafruit_TMF8801::waitForApp(uint16_t timeoutMs) {
  Adafruit_BusIO_Register appId =
      Adafruit_BusIO_Register(_i2c_dev, TMF8801_REG_APPID);
  uint32_t start = millis();
  while ((millis() - start) < timeoutMs) {
    if (appId.read() == TMF8801_APP_MEASUREMENT) {
      return true;
    }
    delay(1);
  }
  return false;
}

bool Adafruit_TMF8801::waitForIdle(uint16_t timeoutMs) {
  Adafruit_BusIO_Register state =
      Adafruit_BusIO_Register(_i2c_dev, TMF8801_REG_STATE);
  uint32_t start = millis();
  while ((millis() - start) < timeoutMs) {
    if (state.read() == TMF8801_STATE_IDLE) {
      return true;
    }
    delay(1);
  }
  return false;
}

bool Adafruit_TMF8801::waitForCommand(uint8_t command, uint16_t timeoutMs) {
  uint32_t start = millis();
  while ((millis() - start) < timeoutMs) {
    uint8_t reg = TMF8801_REG_COMMAND;
    uint8_t commandState[2];
    if (!_i2c_dev->write_then_read(&reg, 1, commandState,
                                   sizeof(commandState))) {
      return false;
    }
    // PREVIOUS is the TMF8801 command-acceptance indicator. COMMAND can retain
    // an older long-running command even after a new command is accepted.
    if (commandState[1] == command) {
      return true;
    }
    delay(1);
  }
  return false;
}

bool Adafruit_TMF8801::clearInterrupt(uint8_t mask) {
  Adafruit_BusIO_Register interruptStatus =
      Adafruit_BusIO_Register(_i2c_dev, TMF8801_REG_INT_STATUS);
  return interruptStatus.write(mask);
}
