// Edge Impulse + Cloud Offloading Wand Sketch
#include <fiona09x-lab4_inferencing.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#define CONFIDENCE_THRESHOLD 80.0
const char* ssid = "UW MSPK";
const char* password = "]6T)/RR--e";
const char* serverUrl = "http://127.0.0.1:8000/"; // Replace with your server

Adafruit_MPU6050 mpu;
float features[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE];
int sample_count = 0;
unsigned long last_sample_time = 0;
unsigned long capture_start_time = 0;
bool capturing = false;

int raw_feature_get_data(size_t offset, size_t length, float *out_ptr) {
  memcpy(out_ptr, features + offset, length * sizeof(float));
  return 0;
}

void setupWiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  mpu.begin();
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  setupWiFi();
  Serial.println("Ready. Send 'o' to start");
}

void loop() {
  if (Serial.available() > 0 && Serial.read() == 'o') {
    sample_count = 0;
    capturing = true;
    capture_start_time = millis();
    last_sample_time = millis();
    Serial.println("Capturing...");
  }
  if (capturing) {
    captureData();
  }
}

void captureData() {
  if (millis() - last_sample_time >= 10 && sample_count < EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE / 3) {
    last_sample_time = millis();
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    int idx = sample_count * 3;
    features[idx] = a.acceleration.x;
    features[idx + 1] = a.acceleration.y;
    features[idx + 2] = a.acceleration.z;
    sample_count++;
  }
  if (millis() - capture_start_time >= 1000) {
    capturing = false;
    run_inference();
  }
}

void run_inference() {
  signal_t signal;
  signal.total_length = EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE;
  signal.get_data = &raw_feature_get_data;
  ei_impulse_result_t result = {};
  EI_IMPULSE_ERROR err = run_classifier(&signal, &result, false);
  if (err != EI_IMPULSE_OK) {
    Serial.println("Classifier failed");
    return;
  }

  float max_value = 0;
  int max_index = -1;
  for (int i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
    if (result.classification[i].value > max_value) {
      max_value = result.classification[i].value;
      max_index = i;
    }
  }

  if (max_index != -1) {
    const char* gesture = ei_classifier_inferencing_categories[max_index];
    float confidence = max_value * 100;
    Serial.printf("Local result: %s (%.2f%%)\n", gesture, confidence);

    if (confidence < CONFIDENCE_THRESHOLD) {
      Serial.println("Low confidence, offloading to server...");
      sendRawDataToServer();
    } else {
      // Local actuation (e.g. LED blink)
      Serial.println("Actuating LED locally.");
      digitalWrite(LED_BUILTIN, HIGH);
      delay(500);
      digitalWrite(LED_BUILTIN, LOW);
    }
  }
}

void sendRawDataToServer() {
  HTTPClient http;
  http.begin(serverUrl);
  http.addHeader("Content-Type", "application/json");

  DynamicJsonDocument doc(1024);
  JsonArray data = doc.createNestedArray("data");
  for (int i = 0; i < EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE; i++) {
    data.add(features[i]);
  }

  String jsonStr;
  serializeJson(doc, jsonStr);
  int code = http.POST(jsonStr);

  Serial.print("Response code: ");
  Serial.println(code);
  if (code > 0) {
    String payload = http.getString();
    Serial.println(payload);
    DynamicJsonDocument resp(256);
    deserializeJson(resp, payload);
    const char* gesture = resp["gesture"];
    float confidence = resp["confidence"];
    Serial.printf("Cloud response: %s (%.2f%%)\n", gesture, confidence);
    // Actuate
    digitalWrite(LED_BUILTIN, HIGH);
    delay(250);
    digitalWrite(LED_BUILTIN, LOW);
  }
  http.end();
}
