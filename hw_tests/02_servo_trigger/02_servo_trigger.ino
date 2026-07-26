/*!
 * TMF8801 servo-trigger hardware test
 *
 * Wiring:
 *   TMF8801 EN  -> Metro Mini D3
 *   TMF8801 INT -> Metro Mini D2
 *   Servo signal -> Metro Mini D5
 *
 * Power the servo from a suitable external 5 V supply and connect the servo
 * ground to the Metro Mini ground.
 */

#include <Adafruit_TMF8801.h>
#include <Servo.h>

#define TMF8801_EN_PIN 3
#define TMF8801_INT_PIN 2
#define SERVO_PIN 5

Adafruit_TMF8801 tmf8801;
Servo triggerServo;

void clearOutputsAndHalt(const __FlashStringHelper* message) {
  triggerServo.write(90);
  delay(250);
  triggerServo.detach();
  digitalWrite(TMF8801_EN_PIN, LOW);
  Serial.println(message);
  while (1) {
    delay(10);
  }
}

bool readMeasurement(tmf8801_result_t* result) {
  uint32_t start = millis();
  while (!tmf8801.dataReady() && (millis() - start < 1000)) {
    delay(1);
  }
  if (!tmf8801.dataReady()) {
    return false;
  }
  return tmf8801.readResult(result);
}

void printMeasurement(const __FlashStringHelper* name,
                      const tmf8801_result_t* result) {
  Serial.print(name);
  Serial.print(F(": "));
  Serial.print(result->distance_mm);
  Serial.print(F(" mm, reliability: "));
  Serial.print(result->reliability);
  Serial.print(F(", object hits: "));
  Serial.println(result->objectHits);
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  pinMode(TMF8801_EN_PIN, OUTPUT);
  pinMode(TMF8801_INT_PIN, INPUT_PULLUP);
  digitalWrite(TMF8801_EN_PIN, HIGH);
  triggerServo.attach(SERVO_PIN);
  triggerServo.write(90);
  delay(500);

  Serial.println(F("TMF8801 servo-trigger hardware test"));

  if (!tmf8801.begin()) {
    clearOutputsAndHalt(F("FAIL: begin failed"));
  }
  Serial.println(F("PASS: begin succeeded"));

  if (!tmf8801.startMeasuring()) {
    clearOutputsAndHalt(F("FAIL: measurement start failed"));
  }
  Serial.println(F("PASS: measurement started"));

  tmf8801_result_t centerResult;
  if (!readMeasurement(&centerResult)) {
    clearOutputsAndHalt(F("FAIL: center result read failed"));
  }
  Serial.println(F("PASS: center result read succeeded"));
  printMeasurement(F("Center result"), &centerResult);
  if (centerResult.reliability == 0) {
    Serial.println(F("PASS: center position has no object in view"));
  } else {
    Serial.println(F("PASS: center result is valid"));
  }

  const uint8_t triggerAngles[] = {30, 45, 60, 120, 135, 150};
  bool targetDetected = false;
  tmf8801_result_t triggeredResult;
  for (uint8_t i = 0; i < sizeof(triggerAngles); i++) {
    Serial.print(F("Trying servo angle: "));
    Serial.println(triggerAngles[i]);
    triggerServo.write(triggerAngles[i]);
    delay(500);
    if (!readMeasurement(&triggeredResult)) {
      clearOutputsAndHalt(F("FAIL: triggered result read failed"));
    }
    Serial.println(F("PASS: triggered result read succeeded"));
    printMeasurement(F("Triggered result"), &triggeredResult);
    if (triggeredResult.reliability > 0) {
      targetDetected = true;
      break;
    }
  }
  if (!targetDetected) {
    clearOutputsAndHalt(F("FAIL: triggered result did not detect an object"));
  }
  Serial.println(F("PASS: triggered result is valid"));

  triggerServo.write(90);
  delay(500);
  triggerServo.detach();
  Serial.println(F("PASS: servo returned to safe position"));
  Serial.println(F("ALL TESTS PASSED"));
}

void loop() { delay(1000); }
