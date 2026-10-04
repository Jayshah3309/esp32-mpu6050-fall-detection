#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <math.h>

// Copy secrets.example.h to secrets.h to enable optional Blynk reporting.
#if __has_include("secrets.h")
#include "secrets.h"
#define FALL_BLYNK_ENABLED 1
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#else
#define FALL_BLYNK_ENABLED 0
#endif

namespace Config {
constexpr uint32_t SAMPLE_INTERVAL_MS = 50;
constexpr uint32_t STILLNESS_CONFIRM_MS = 1200;
constexpr uint32_t ALERT_CANCEL_WINDOW_MS = 20000;
constexpr uint32_t ALERT_REPEAT_COOLDOWN_MS = 60000;
constexpr float IMPACT_THRESHOLD_G = 2.5f;
constexpr float STILLNESS_DYNAMIC_G = 0.35f;
constexpr float POSTURE_CHANGE_DEGREES = 55.0f;
}  // namespace Config

Adafruit_MPU6050 mpu;
uint32_t lastSampleMs = 0;
uint32_t lastTelemetryMs = 0;
uint32_t lastNetworkAttemptMs = 0;
uint32_t impactMs = 0;
uint32_t stillSinceMs = 0;
uint32_t alertStartedMs = 0;
uint32_t lastAlertMs = 0;
float baselineTiltDegrees = 0.0f;
bool baselineReady = false;
bool impactPending = false;
bool fallAlertPending = false;

float accelerationMagnitudeG(const sensors_event_t& a) {
  const float magnitude = sqrtf(a.acceleration.x * a.acceleration.x +
                                a.acceleration.y * a.acceleration.y +
                                a.acceleration.z * a.acceleration.z);
  return magnitude / SENSORS_GRAVITY_STANDARD;
}

float tiltDegrees(const sensors_event_t& a) {
  const float magnitude = sqrtf(a.acceleration.x * a.acceleration.x +
                                a.acceleration.y * a.acceleration.y +
                                a.acceleration.z * a.acceleration.z);
  if (magnitude < 0.1f) return 0.0f;
  const float verticalRatio = constrain(fabsf(a.acceleration.z) / magnitude, 0.0f, 1.0f);
  return acosf(verticalRatio) * 180.0f / PI;
}

void reportFallEvent() {
  Serial.println("Fall event confirmed. Check the wearer.");
#if FALL_BLYNK_ENABLED
  Blynk.virtualWrite(V6, 1);
  Blynk.logEvent("fall_detected", "Possible fall detected. Check the wearer.");
#endif
}

void beginFallAlert(uint32_t now) {
  fallAlertPending = true;
  alertStartedMs = now;
  Serial.println("Possible fall is pending. Send cancel on Blynk V7 within 20 seconds if this is a false alarm.");
#if FALL_BLYNK_ENABLED
  Blynk.virtualWrite(V6, 1);
#endif
}

void cancelFallAlert() {
  if (!fallAlertPending) return;
  fallAlertPending = false;
  impactPending = false;
  stillSinceMs = 0;
  Serial.println("Fall alert cancelled by user.");
#if FALL_BLYNK_ENABLED
  Blynk.virtualWrite(V6, 0);
#endif
}

void processSample(uint32_t now, const sensors_event_t& a, const sensors_event_t& g) {
  const float magnitudeG = accelerationMagnitudeG(a);
  const float tilt = tiltDegrees(a);
  const float gyroMagnitude = sqrtf(g.gyro.x * g.gyro.x + g.gyro.y * g.gyro.y +
                                   g.gyro.z * g.gyro.z);

  if (!baselineReady && magnitudeG > 0.85f && magnitudeG < 1.15f) {
    baselineTiltDegrees = tilt;
    baselineReady = true;
    Serial.println("Initial posture captured. Wear the device in its normal position.");
  }

  if (impactPending) {
    const float dynamicG = fabsf(magnitudeG - 1.0f);
    const bool postureChanged = baselineReady &&
        fabsf(tilt - baselineTiltDegrees) >= Config::POSTURE_CHANGE_DEGREES;
    const bool isStill = dynamicG <= Config::STILLNESS_DYNAMIC_G && gyroMagnitude < 1.2f;

    if (isStill && postureChanged) {
      if (stillSinceMs == 0) stillSinceMs = now;
      if (now - stillSinceMs >= Config::STILLNESS_CONFIRM_MS && !fallAlertPending &&
          (lastAlertMs == 0 || now - lastAlertMs >= Config::ALERT_REPEAT_COOLDOWN_MS)) {
        beginFallAlert(now);
        lastAlertMs = now;
      }
    } else {
      stillSinceMs = 0;
    }

    if (now - impactMs > 5000) {
      impactPending = false;
      stillSinceMs = 0;
    }
  }

  if (magnitudeG >= Config::IMPACT_THRESHOLD_G && !fallAlertPending &&
      (lastAlertMs == 0 || now - lastAlertMs >= Config::ALERT_REPEAT_COOLDOWN_MS)) {
    impactPending = true;
    impactMs = now;
    stillSinceMs = 0;
    Serial.println("High impact detected; checking motion and posture before alerting.");
  }

#if FALL_BLYNK_ENABLED
  if (now - lastTelemetryMs >= 200) {
    lastTelemetryMs = now;
    Blynk.virtualWrite(V0, a.acceleration.x);
    Blynk.virtualWrite(V1, a.acceleration.y);
    Blynk.virtualWrite(V2, a.acceleration.z);
    Blynk.virtualWrite(V3, g.gyro.x);
    Blynk.virtualWrite(V4, g.gyro.y);
    Blynk.virtualWrite(V5, g.gyro.z);
  }
#endif
}

#if FALL_BLYNK_ENABLED
BLYNK_WRITE(V7) {
  if (param.asInt() != 0) cancelFallAlert();
}
#endif

void setup() {
  Serial.begin(115200);
  Wire.begin();
  if (!mpu.begin()) {
    Serial.println("MPU6050 not found. Check power, I2C wiring, and address.");
    while (true) delay(1000);
  }

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  Serial.println("MPU6050 ready. Keep the device still in its normal wearing position for startup calibration.");

#if FALL_BLYNK_ENABLED
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Blynk.config(BLYNK_AUTH_TOKEN);
  Serial.println("Blynk reporting configured; sensor detection continues while the network reconnects.");
#else
  Serial.println("Blynk disabled: add private credentials in secrets.h to enable remote telemetry and alerts.");
#endif
}

void loop() {
#if FALL_BLYNK_ENABLED
  const uint32_t now = millis();
  if (Blynk.connected()) {
    Blynk.run();
  } else if (WiFi.status() == WL_CONNECTED && now - lastNetworkAttemptMs >= 10000) {
    lastNetworkAttemptMs = now;
    Blynk.connect(100);
  } else if (WiFi.status() != WL_CONNECTED && now - lastNetworkAttemptMs >= 10000) {
    lastNetworkAttemptMs = now;
    WiFi.reconnect();
  }
#endif
  const uint32_t now = millis();
  if (now - lastSampleMs >= Config::SAMPLE_INTERVAL_MS) {
    lastSampleMs = now;
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    processSample(now, a, g);
  }
  if (fallAlertPending && now - alertStartedMs >= Config::ALERT_CANCEL_WINDOW_MS) {
    fallAlertPending = false;
    reportFallEvent();
  }
}
