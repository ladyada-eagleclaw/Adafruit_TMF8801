/*!
 * TMF8801 GPIO output toggle test
 *
 * Wiring:
 *   TMF8801 EN  -> Metro Mini D3
 *   TMF8801 INT -> Metro Mini D2
 *
 * Leave GPIO0 and GPIO1 disconnected from the Metro Mini. Probe each GPIO
 * relative to GND. The outputs use the TMF8801 VDD logic level; do not apply an
 * external voltage to either pin.
 */

#include <Adafruit_TMF8801.h>

const uint16_t TMF8801_EN_PIN = 3;
const uint16_t TMF8801_INT_PIN = 2;
const uint16_t TOGGLE_INTERVAL_MS = 1000;

Adafruit_TMF8801 tmf8801;
bool gpio0High = true;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println(F("TMF8801 GPIO output toggle test"));

  pinMode(TMF8801_EN_PIN, OUTPUT);
  pinMode(TMF8801_INT_PIN, INPUT_PULLUP);
  digitalWrite(TMF8801_EN_PIN, HIGH);
  delay(10);

  if (!tmf8801.begin()) {
    clearOutputsAndHalt(F("FAIL: begin failed"));
  }
  Serial.println(F("PASS: begin succeeded"));
  Serial.println(F("GPIO0 and GPIO1 will alternate every second"));

  applyGPIOLevels();
}

void loop() {
  delay(TOGGLE_INTERVAL_MS);

  if (!tmf8801.stopMeasuring()) {
    clearOutputsAndHalt(F("FAIL: measurement stop failed"));
  }

  gpio0High = !gpio0High;
  applyGPIOLevels();
}

void applyGPIOLevels() {
  tmf8801_gpio_mode_t expectedGPIO0;
  tmf8801_gpio_mode_t expectedGPIO1;
  if (gpio0High) {
    expectedGPIO0 = TMF8801_GPIO_OUTPUT_HIGH;
    expectedGPIO1 = TMF8801_GPIO_OUTPUT_LOW;
  } else {
    expectedGPIO0 = TMF8801_GPIO_OUTPUT_LOW;
    expectedGPIO1 = TMF8801_GPIO_OUTPUT_HIGH;
  }

  tmf8801.setGPIOMode(0, expectedGPIO0);
  tmf8801.setGPIOMode(1, expectedGPIO1);
  if (tmf8801.getGPIOMode(0) != expectedGPIO0 ||
      tmf8801.getGPIOMode(1) != expectedGPIO1) {
    clearOutputsAndHalt(F("FAIL: GPIO modes did not read back correctly"));
  }

  // GPIO modes are applied when a measurement command starts. Continuous mode
  // keeps the selected levels active until the next update.
  if (!tmf8801.startMeasuring(true)) {
    clearOutputsAndHalt(F("FAIL: measurement start failed"));
  }

  if (gpio0High) {
    Serial.println(F("GPIO0 HIGH   GPIO1 LOW"));
  } else {
    Serial.println(F("GPIO0 LOW    GPIO1 HIGH"));
  }
}

void clearOutputsAndHalt(const __FlashStringHelper *message) {
  digitalWrite(TMF8801_EN_PIN, LOW);
  Serial.println(message);
  while (1) {
    delay(10);
  }
}
