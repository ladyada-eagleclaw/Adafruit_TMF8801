/*!
 * @file Adafruit_TMF8801.h
 *
 * Arduino library for the ams OSRAM TMF8801 Time-of-Flight sensor.
 *
 * Adafruit invests time and resources providing this open source code.
 * Please support Adafruit and open-source hardware by purchasing products
 * from Adafruit!
 *
 * Written by Limor Fried/Ladyada for Adafruit Industries.
 *
 * MIT license, all text above must be included in any redistribution.
 */

#ifndef ADAFRUIT_TMF8801_H
#define ADAFRUIT_TMF8801_H

#include <Adafruit_BusIO_Register.h>
#include <Adafruit_I2CDevice.h>
#include <Arduino.h>

#define TMF8801_DEFAULT_ADDR 0x41 ///< Default I2C address

#define TMF8801_REG_APPID 0x00         ///< Application ID
#define TMF8801_REG_APPREV_MAJOR 0x01  ///< Application major version
#define TMF8801_REG_APPREQID 0x02      ///< Requested application ID
#define TMF8801_REG_CMD_DATA9 0x06     ///< First capture command byte
#define TMF8801_REG_BOOTLOADER 0x08    ///< Bootloader command and status
#define TMF8801_REG_COMMAND 0x10       ///< Command register
#define TMF8801_REG_PREVIOUS 0x11      ///< Previous command register
#define TMF8801_REG_APPREV_MINOR 0x12  ///< Application minor version
#define TMF8801_REG_APPREV_PATCH 0x13  ///< Application patch version
#define TMF8801_REG_STATE 0x1C         ///< Application state
#define TMF8801_REG_STATUS 0x1D        ///< Application status
#define TMF8801_REG_CONTENTS 0x1E      ///< Published content type
#define TMF8801_REG_RESULT_NUMBER 0x20 ///< First result or calibration byte
#define TMF8801_REG_ENABLE 0xE0        ///< CPU status and control
#define TMF8801_REG_INT_STATUS 0xE1    ///< Interrupt status
#define TMF8801_REG_INT_ENABLE 0xE2    ///< Interrupt enable
#define TMF8801_REG_ID 0xE3            ///< Chip ID
#define TMF8801_REG_REVID 0xE4         ///< Hardware revision

#define TMF8801_APP_BOOTLOADER 0x80  ///< Bootloader application ID
#define TMF8801_APP_MEASUREMENT 0xC0 ///< Measurement application ID
#define TMF8801_CHIP_ID 0x07         ///< Expected chip ID

#define TMF8801_ENABLE_POWER_ON 0x01  ///< Power-on control flag
#define TMF8801_ENABLE_CPU_READY 0x40 ///< CPU-ready status flag
#define TMF8801_ENABLE_RESET 0x80     ///< CPU reset control flag
#define TMF8801_ENABLE_POWER_ON_BIT 0 ///< Power-on control bit position

#define TMF8801_BL_UPLOAD_INIT 0x14    ///< Initialize firmware upload
#define TMF8801_BL_WRITE_RAM 0x41      ///< Write firmware data to RAM
#define TMF8801_BL_ADDRESS_RAM 0x43    ///< Set RAM write address
#define TMF8801_BL_RAM_REMAP 0x11      ///< Run uploaded RAM firmware
#define TMF8801_BL_BUSY 0x10           ///< Bootloader busy response
#define TMF8801_BL_READY 0x00          ///< Bootloader ready response
#define TMF8801_BL_CHUNK_SIZE 16       ///< Firmware upload chunk size
#define TMF8801_BL_DEFAULT_SALT 0x29   ///< Firmware upload salt
#define TMF8801_BL_RAM_ADDRESS 0x0000  ///< Firmware RAM base address
#define TMF8801_BL_NO_DATA 0           ///< Empty bootloader payload length
#define TMF8801_BL_VALID_CHECKSUM 0xFF ///< Valid response checksum

#define TMF8801_CMD_MEASURE_WITH_CAL 0x02 ///< Start using supplied data
#define TMF8801_CMD_MEASURE 0x03          ///< Start without calibration
#define TMF8801_CMD_FACTORY_CALIB 0x0A    ///< Factory calibration
#define TMF8801_CMD_READ_SERIAL 0x47      ///< Read serial number
#define TMF8801_CMD_STOP 0xFF             ///< Stop capture

#define TMF8801_CONTENT_CALIBRATION 0x0A ///< Factory calibration result
#define TMF8801_CONTENT_RESULT 0x55      ///< Distance result

#define TMF8801_STATE_IDLE 0x01 ///< Application idle state

#define TMF8801_INT_RESULT 0x01 ///< Result interrupt mask
#define TMF8801_INT_ERROR 0x04  ///< Error interrupt mask

#define TMF8801_DATA_FACTORY_CALIBRATION 0x01 ///< Factory calibration flag
#define TMF8801_DATA_ALGORITHM_STATE 0x02     ///< Algorithm state flag
#define TMF8801_ALGORITHM_DEFAULT 0x23        ///< Default algorithm settings
#define TMF8801_GPIO_MODE_MASK 0x0F           ///< One GPIO mode field
#define TMF8801_GPIO1_MODE_SHIFT 4            ///< GPIO 1 mode field shift
#define TMF8801_RESULT_RELIABILITY_MASK 0x3F  ///< Result reliability field
#define TMF8801_RESULT_STATUS_SHIFT 6         ///< Result status field shift

#define TMF8801_CALIBRATION_DATA_SIZE 14 ///< Factory calibration byte count
#define TMF8801_ALGORITHM_STATE_SIZE 11  ///< Algorithm state byte count
#define TMF8801_RESULT_DATA_SIZE 27      ///< Measurement result byte count

/*!
 * @brief TMF8801 GPIO function used during capture
 */
typedef enum {
  TMF8801_GPIO_DISABLED = 0,          ///< GPIO disabled
  TMF8801_GPIO_INPUT_ACTIVE_LOW = 1,  ///< Low level pauses capture
  TMF8801_GPIO_INPUT_ACTIVE_HIGH = 2, ///< High level pauses capture
  TMF8801_GPIO_OUTPUT_VCSEL = 3,      ///< VCSEL timing output
  TMF8801_GPIO_OUTPUT_LOW = 4,        ///< Drive low
  TMF8801_GPIO_OUTPUT_HIGH = 5,       ///< Drive high
} tmf8801_gpio_mode_t;

/*!
 * @brief One complete TMF8801 measurement result
 */
typedef struct {
  uint8_t number;         ///< Rolling result number
  uint16_t distance_mm;   ///< Distance in millimeters
  uint8_t reliability;    ///< Confidence from 0 (invalid) to 63 (best)
  uint8_t status;         ///< Result status from bits 7:6 of result info
  uint32_t systemClock;   ///< Sensor clock captured with this result
  uint32_t referenceHits; ///< Reference SPAD hit count
  uint32_t objectHits;    ///< Object SPAD hit count
} tmf8801_result_t;

/*!
 * @brief Driver for the ams OSRAM TMF8801 Time-of-Flight sensor
 */
class Adafruit_TMF8801 {
 public:
  Adafruit_TMF8801();
  ~Adafruit_TMF8801();

  bool begin(uint8_t addr = TMF8801_DEFAULT_ADDR, TwoWire* wire = &Wire);
  bool reset();

  bool startMeasuring(bool continuous = true);
  bool stopMeasuring();
  bool dataReady();
  int16_t readDistance();
  bool readResult(tmf8801_result_t* result);

  void setIterations(uint16_t kiloIterations);
  uint16_t getIterations();
  void setRepetitionPeriod(uint8_t periodMs);
  uint8_t getRepetitionPeriod();
  void setNoiseThreshold(uint8_t threshold);
  uint8_t getNoiseThreshold();
  void setGPIOMode(uint8_t gpio, tmf8801_gpio_mode_t mode);
  tmf8801_gpio_mode_t getGPIOMode(uint8_t gpio);

  bool performFactoryCalibration();
  bool getCalibrationData(uint8_t* data,
                          uint8_t len = TMF8801_CALIBRATION_DATA_SIZE);
  bool setCalibrationData(const uint8_t* data,
                          uint8_t len = TMF8801_CALIBRATION_DATA_SIZE);
  void enableCalibration(bool enable);

  bool getAlgorithmState(uint8_t* data,
                         uint8_t len = TMF8801_ALGORITHM_STATE_SIZE);
  bool setAlgorithmState(const uint8_t* data,
                         uint8_t len = TMF8801_ALGORITHM_STATE_SIZE);
  void enableAlgorithmState(bool enable);

  uint8_t getChipID();
  uint8_t getRevisionID();
  uint8_t getStatus();
  void getVersion(uint8_t* major, uint8_t* minor, uint8_t* patch);
  bool readSerialNumber(uint16_t* serialNumber);

  bool sleep();
  bool wakeup();

 private:
  Adafruit_I2CDevice* _i2c_dev; ///< BusIO I2C device
  uint16_t _iterations;         ///< Thousands of integration iterations
  uint8_t _repetitionPeriod;    ///< Continuous result period in milliseconds
  uint8_t _noiseThreshold;      ///< Result noise threshold
  uint8_t _gpio0Mode;           ///< GPIO0 capture mode
  uint8_t _gpio1Mode;           ///< GPIO1 capture mode
  bool _useCalibration;         ///< Supply factory calibration on start
  bool _hasCalibration;         ///< Stored calibration is valid
  bool _useAlgorithmState;      ///< Supply algorithm state on start
  bool _hasAlgorithmState;      ///< Stored algorithm state is valid
  uint8_t _calibrationData[TMF8801_CALIBRATION_DATA_SIZE];
  uint8_t _algorithmState[TMF8801_ALGORITHM_STATE_SIZE];

  bool startApp();
  bool uploadFirmware();
  bool sendBootloaderCommand(uint8_t command, const uint8_t* data, uint8_t len);
  bool waitForBootloader(uint16_t timeoutMs);
  bool waitForCpuReady(uint16_t timeoutMs);
  bool waitForApp(uint16_t timeoutMs);
  bool waitForIdle(uint16_t timeoutMs);
  bool waitForCommand(uint8_t command, uint16_t timeoutMs);
  bool clearInterrupt(uint8_t mask);
};

#endif // ADAFRUIT_TMF8801_H
