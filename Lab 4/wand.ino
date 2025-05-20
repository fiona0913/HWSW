#include <Arduino.h>
#include <fiona09x-lab4_inferencing.h> // Edge Impulse model file
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

// pin definition
#define RED_LED    D8
#define GREEN_LED  D9
#define BLUE_LED   D10
#define BUTTON_PIN D2

// sensor
Adafruit_MPU6050 mpu;

// data sampling params
#define SAMPLE_RATE_MS 10
#define CAPTURE_DURATION_MS 1000

// fatures data
float features[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE];

// states
bool capturing = false;
bool buttonPressed = false;
unsigned long last_sample_time = 0;
unsigned long capture_start_time = 0;
int sample_count = 0;

// data reading
int raw_feature_get_data(size_t offset, size_t length, float *out_ptr) {
  memcpy(out_ptr, features + offset, length * sizeof(float));
  return 0;
}

// LED
void lightLED(const char* label) {
  if (strcmp(label, "O") == 0) {
    digitalWrite(RED_LED, HIGH);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(BLUE_LED, LOW);
  } else if (strcmp(label, "V") == 0) {
    digitalWrite(RED_LED, LOW);
    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(BLUE_LED, LOW);
  } else if (strcmp(label, "Z") == 0) {
    digitalWrite(RED_LED, LOW);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(BLUE_LED, HIGH);
  } else {
    // 未识别/熄灭所有灯
    digitalWrite(RED_LED, LOW);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(BLUE_LED, LOW);
  }
}

// Initialization
void setup() {
  Serial.begin(115200);

  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP); // 内部上拉，按钮接地有效

  // Initialize MPU6050
  if (!mpu.begin()) {
    Serial.println("MPU6050 not found!");
    while (1) delay(10);
  }

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  Serial.println("Setup complete. Press button to recognize gesture.");
}

// Data collection and calculation
void capture_accelerometer_data() {
  if (millis() - last_sample_time >= SAMPLE_RATE_MS) {
    last_sample_time = millis();

    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    if (sample_count < EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE / 3) {
      int idx = sample_count * 3;
      features[idx] = a.acceleration.x;
      features[idx + 1] = a.acceleration.y;
      features[idx + 2] = a.acceleration.z;
      sample_count++;
    }

    if (millis() - capture_start_time >= CAPTURE_DURATION_MS) {
      capturing = false;

      ei_impulse_result_t result = { 0 };
      signal_t signal;
      signal.total_length = EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE;
      signal.get_data = &raw_feature_get_data;

      EI_IMPULSE_ERROR res = run_classifier(&signal, &result, false);
      if (res != EI_IMPULSE_OK) {
        Serial.println("Failed to run classifier.");
        return;
      }

      // Most probable class
      float max_val = 0;
      int max_idx = -1;
      for (int i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        if (result.classification[i].value > max_val) {
          max_val = result.classification[i].value;
          max_idx = i;
        }
      }

      if (max_idx >= 0) {
        const char* label = ei_classifier_inferencing_categories[max_idx];
        Serial.print("Detected gesture: ");
        Serial.print(label);
        Serial.print(" (");
        Serial.print(max_val * 100);
        Serial.println("%)");

        lightLED(label);  // LED ON
        delay(1000);      // ON for 1 sec 
        lightLED("off");  // LED OFF
      }
    }
  }
}

// Main loop
void loop() {
  // Detect if button is pressed
  if (digitalRead(BUTTON_PIN) == LOW && !buttonPressed) {
    buttonPressed = true;
    capturing = true;
    sample_count = 0;
    capture_start_time = millis();
    last_sample_time = millis();
    Serial.println("Button pressed! Start capturing...");
  }

  // Reset button
  if (digitalRead(BUTTON_PIN) == HIGH) {
    buttonPressed = false;
  }

  // Keep reading data if in capture mode
  if (capturing) {
    capture_accelerometer_data();
  }
}