# Wiring and configuration notes

Use the pin labels on the specific ESP32 board and MPU6050 breakout. The firmware uses Arduino `Wire` I2C and the board's default SDA/SCL pins. Breakout boards differ in regulator and logic-level support, so verify the module documentation before choosing VIN voltage.

The original circuit image names a NodeMCU board, while the firmware targets ESP32. Confirm the exact board before wiring; do not assume ESP8266 NodeMCU pin labels apply to an ESP32.

The Blynk event name must be configured as `fall_detected`. Virtual pins V0–V5 carry accelerometer and gyroscope values, V6 indicates a pending alert, and V7 accepts a cancellation button press. Blynk is optional: without the private `fall_detection/secrets.h` file, Serial output remains available.

Thresholds and timings are declared near the top of `fall_detection/fall_detection.ino`. Adjust them only after collecting safe, labelled data. This heuristic is an early prototype and has not been validated for real-world or medical use.
