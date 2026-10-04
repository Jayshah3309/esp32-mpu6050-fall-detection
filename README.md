# ESP32 MPU6050 Fall Detection Prototype

An experimental wearable fall-detection prototype using an ESP32 and an MPU6050. The firmware samples acceleration and angular velocity, looks for a high-impact event followed by stillness and a change in posture, and reports a possible fall. Blynk reporting is optional; the sensor algorithm also runs without network credentials.

> **Safety and validation:** This is a prototype, not a medical or emergency-response device. The thresholds are starting values and have not been clinically validated. Do not rely on it to protect a person. Tune and evaluate it with safe, controlled tests before any supervised demonstration.

## Repository layout

- `fall_detection/fall_detection.ino` — Arduino sketch for the ESP32.
- `fall_detection/secrets.example.h` — sample configuration with placeholders.
- `docs/` — circuit image, original project report draft, and setup notes.

## Hardware

- ESP32 development board
- MPU6050 breakout board
- USB power for development

Connect the sensor using the board's I2C pins: SDA to SDA, SCL to SCL, GND to GND, and VIN to a voltage supported by the specific breakout. ESP32 pin assignments vary by board; check its pinout before wiring. The supplied circuit image is a reference, not a verified wiring specification for every ESP32 board. Confirm the exact board and sensor breakout before powering the circuit.

## Build and upload

1. Install Arduino IDE and the ESP32 board package.
2. Install **Adafruit MPU6050** and its dependencies (**Adafruit Unified Sensor** and **Adafruit BusIO**) through Library Manager.
3. Open `fall_detection/fall_detection.ino`, select the matching ESP32 board and port, then upload.
4. Open Serial Monitor at 115200 baud. Keep the sensor still in its normal wearing position at startup so the firmware can capture the initial posture.

Without `secrets.h`, the sketch runs its sensor algorithm and reports status to Serial. To enable Blynk, copy `fall_detection/secrets.example.h` to `fall_detection/secrets.h`, enter a newly issued Blynk device token and Wi-Fi credentials, and upload again. `secrets.h` is ignored by Git. Rotate any credentials that were previously committed publicly before using them again.

## Blynk setup

Create a Blynk template/device and an event named `fall_detected`. Configure datastreams V0–V5 as numeric values for the three acceleration and three gyro axes. V6 is the possible-fall indicator; V7 is a push button that sends `1` to cancel a pending alert. If the user does not cancel within 20 seconds, the firmware logs the Blynk event.

## Detection flow and known limits

The initial implementation uses configurable thresholds in the sketch: an acceleration magnitude of at least 2.5 g starts a candidate, then a posture change of at least 55 degrees and low motion sustained for 1.2 seconds confirms it. A 20-second cancellation window follows, and a 60-second cooldown suppresses repeat detections. Blynk telemetry is sent at 5 Hz per value when connected; sensor sampling and local detection continue during network reconnects. Remote notifications can be lost while offline because events are not queued or persisted. These values are provisional. Device placement, individual movement, impact cushioning, sensor mounting, and board variation can cause missed detections or false alarms. The current algorithm has no buzzer, cellular/SMS alert, GPS, battery monitoring, persistent local event log, or clinical validation.

## Calibration and evaluation

Before tuning thresholds, record the sensor's normal orientation and collect labelled, safe examples of everyday activities. Evaluate only with an instrumented dummy or safely controlled non-human setup; do not ask a person to perform falls. Include device drops separately from wearer-fall scenarios, since they can look similar to an impact sensor. Report sample counts, false positives, missed events, and detection delay. Keep the report's results section blank until measurements have actually been collected.

## Project report

`docs/fall_detection_project_report_draft.docx` is the original academic report draft. It contains template fields and describes design ideas that are not implemented here. Treat it as an unverified draft: fill identity fields only with correct details, update feature claims to match the firmware, and add measured results and references before presenting it as a completed project report.
