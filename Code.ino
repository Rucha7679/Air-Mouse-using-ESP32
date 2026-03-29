#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <BleMouse.h>

Adafruit_MPU6050 mpu;
BleMouse bleMouse("ESP32 Air Mouse", "ESP32", 100);

// LDR setup
const int LDR_PIN = 34;   // ADC pin (use GPIO34, 35, 36, or 39 for analog input)
int lightValue = 0;
const int CLICK_THRESHOLD = 500;  // Adjust depending on your LDR and lighting

// Calibration offsets
float gyroXoffset = 0;
float gyroYoffset = 0;

// Gyro calibration
void calibrateGyro() {
  Serial.println("Calibrating gyro... Keep the sensor still!");
  long sumX = 0, sumY = 0;
  int samples = 1000;

  for (int i = 0; i < samples; i++) {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    sumX += g.gyro.x;
    sumY += g.gyro.y;
    delay(3);
  }

  gyroXoffset = sumX / (float)samples;
  gyroYoffset = sumY / (float)samples;
  Serial.println("Calibration done!");
  Serial.print("Offsets -> X: "); Serial.print(gyroXoffset);
  Serial.print("  Y: "); Serial.println(gyroYoffset);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting ESP32 Air Mouse...");

  // Initialize MPU6050
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip!");
    while (1) delay(10);
  }
  Serial.println("MPU6050 initialized.");

  // Configure sensor ranges
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  delay(500);
  calibrateGyro();

  // Initialize BLE Mouse
  bleMouse.begin();
  Serial.println("BLE Mouse ready. Connect from your PC via Bluetooth!");

  // Initialize LDR
  pinMode(LDR_PIN, INPUT);
}

void loop() {
  if (bleMouse.isConnected()) {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    // Apply calibration
    float gx = g.gyro.x - gyroXoffset;
    float gy = g.gyro.y - gyroYoffset;

    // Dead zone
    if (abs(gx) < 0.5) gx = 0;
    if (abs(gy) < 0.5) gy = 0;

    // Smoothing
    static float smoothedX = 0, smoothedY = 0;
    smoothedX = 0.8 * smoothedX + 0.2 * gx;
    smoothedY = 0.8 * smoothedY + 0.2 * gy;

    // Cursor movement
    int moveX = (int)(smoothedX * 8);
    int moveY = (int)(-smoothedY * 8);

    if (moveX != 0 || moveY != 0) {
      bleMouse.move(moveX, moveY);
    }

    // --- LDR Click Section ---
    lightValue = analogRead(LDR_PIN);
    Serial.print("LDR Value: ");
    Serial.println(lightValue);

    // If dark, simulate a click
    if (lightValue > CLICK_THRESHOLD) {
      bleMouse.click(MOUSE_LEFT);
      delay(200);  // debounce delay
    }

    delay(20);
  } else {
    Serial.println("Waiting for BLE connection...");
    delay(1000);
  }
}
