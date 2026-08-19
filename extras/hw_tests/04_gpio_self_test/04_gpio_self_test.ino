/*!
 * TMF8801 GPIO self-test
 *
 * Wiring:
 *   TMF8801 EN    -> Metro Mini D3
 *   TMF8801 INT   -> Metro Mini D2
 *   TMF8801 GPIO0 -> 1 kOhm resistor -> TMF8801 GPIO1
 *
 * A direct GPIO0-to-GPIO1 jumper also works, but the resistor protects against
 * accidental output contention. Do not drive either TMF8801 GPIO from a 5 V
 * Metro output; the GPIO voltage must not exceed the sensor's VDD by 0.3 V.
 */

#include <Adafruit_TMF8801.h>

const uint16_t TMF8801_EN_PIN = 3;
const uint16_t TMF8801_INT_PIN = 2;
const uint16_t RESULT_TIMEOUT_MS = 1000;
const uint16_t BLOCKED_TEST_TIME_MS = 250;

Adafruit_TMF8801 tmf8801;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println(F("TMF8801 GPIO self-test"));

  pinMode(TMF8801_EN_PIN, OUTPUT);
  pinMode(TMF8801_INT_PIN, INPUT_PULLUP);
  digitalWrite(TMF8801_EN_PIN, HIGH);
  delay(10);

  if (!tmf8801.begin()) {
    clearOutputsAndHalt(F("FAIL: begin failed"));
  }
  Serial.println(F("PASS: begin succeeded"));

  tmf8801.setIterations(900);

  runGPIOCase(F("GPIO0 low, GPIO1 active high"), TMF8801_GPIO_OUTPUT_LOW,
              TMF8801_GPIO_INPUT_ACTIVE_HIGH, true);
  runGPIOCase(F("GPIO0 high, GPIO1 active high"), TMF8801_GPIO_OUTPUT_HIGH,
              TMF8801_GPIO_INPUT_ACTIVE_HIGH, false);
  runGPIOCase(F("GPIO0 high, GPIO1 active low"), TMF8801_GPIO_OUTPUT_HIGH,
              TMF8801_GPIO_INPUT_ACTIVE_LOW, true);
  runGPIOCase(F("GPIO0 low, GPIO1 active low"), TMF8801_GPIO_OUTPUT_LOW,
              TMF8801_GPIO_INPUT_ACTIVE_LOW, false);

  runGPIOCase(F("GPIO1 low, GPIO0 active high"),
              TMF8801_GPIO_INPUT_ACTIVE_HIGH, TMF8801_GPIO_OUTPUT_LOW, true);
  runGPIOCase(F("GPIO1 high, GPIO0 active high"),
              TMF8801_GPIO_INPUT_ACTIVE_HIGH, TMF8801_GPIO_OUTPUT_HIGH, false);
  runGPIOCase(F("GPIO1 high, GPIO0 active low"),
              TMF8801_GPIO_INPUT_ACTIVE_LOW, TMF8801_GPIO_OUTPUT_HIGH, true);
  runGPIOCase(F("GPIO1 low, GPIO0 active low"),
              TMF8801_GPIO_INPUT_ACTIVE_LOW, TMF8801_GPIO_OUTPUT_LOW, false);

  digitalWrite(TMF8801_EN_PIN, LOW);
  Serial.println();
  Serial.println(F("PASS: sensor disabled"));
  Serial.println(F("ALL TESTS PASSED"));
}

void loop() { delay(1000); }

void runGPIOCase(const __FlashStringHelper *description,
                 tmf8801_gpio_mode_t gpio0Mode,
                 tmf8801_gpio_mode_t gpio1Mode, bool expectMeasurement) {
  Serial.println();
  Serial.print(F("TEST: "));
  Serial.println(description);

  tmf8801.setGPIOMode(0, gpio0Mode);
  tmf8801.setGPIOMode(1, gpio1Mode);
  if (tmf8801.getGPIOMode(0) != gpio0Mode ||
      tmf8801.getGPIOMode(1) != gpio1Mode) {
    clearOutputsAndHalt(F("FAIL: GPIO modes did not read back correctly"));
  }
  Serial.println(F("PASS: GPIO modes read back correctly"));

  if (!tmf8801.startMeasuring(true)) {
    clearOutputsAndHalt(F("FAIL: measurement start failed"));
  }
  Serial.println(F("PASS: measurement started"));

  if (expectMeasurement) {
    if (!waitForData(RESULT_TIMEOUT_MS)) {
      clearOutputsAndHalt(F("FAIL: inactive GPIO input blocked measurement"));
    }
    Serial.println(F("PASS: inactive GPIO input allowed measurement"));

    tmf8801_result_t result;
    if (!tmf8801.readResult(&result)) {
      clearOutputsAndHalt(F("FAIL: result read failed"));
    }
    Serial.println(F("PASS: result read succeeded"));
    if (result.status != TMF8801_MEASUREMENT_NOT_INTERRUPTED) {
      clearOutputsAndHalt(F("FAIL: measurement status was not normal"));
    }
    Serial.println(F("PASS: measurement status was normal"));
  } else {
    uint32_t start = millis();
    while ((millis() - start) < BLOCKED_TEST_TIME_MS) {
      if (tmf8801.dataReady()) {
        clearOutputsAndHalt(
            F("FAIL: asserted GPIO input did not block measurement"));
      }
      delay(1);
    }
    Serial.println(F("PASS: asserted GPIO input blocked measurement"));
  }

  if (!tmf8801.stopMeasuring()) {
    clearOutputsAndHalt(F("FAIL: measurement stop failed"));
  }
  Serial.println(F("PASS: measurement stopped"));
  Serial.println(F("PASS: GPIO case passed"));
}

bool waitForData(uint16_t timeoutMs) {
  uint32_t start = millis();
  while (!tmf8801.dataReady() && (millis() - start < timeoutMs)) {
    delay(1);
  }
  return tmf8801.dataReady();
}

void clearOutputsAndHalt(const __FlashStringHelper *message) {
  digitalWrite(TMF8801_EN_PIN, LOW);
  Serial.println(message);
  while (1) {
    delay(10);
  }
}
