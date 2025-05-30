# Lab 5: Edge + Cloud Gesture Inference

## 📂 Project Structure

```
.
├── ESP32_to_cloud/             # ESP32 Arduino code
│   └── ESP32_to_cloud.ino      # Main ESP32 sketch
├── trainer_scripts             # Scripts
│   ├── train.ipynb             # Model training script
│   └── model_register.ipynb    # Model register script
├── app/                        # Web app for model deployment
│   ├── wand_model.h5           # trained model
│   ├── app.py                  # Script of web app
│   └── requirements.txt        # Dependencies required by web app
└── data/                       # Training data directory
    ├── O/                      # O-shape gesture samples
    ├── V/                      # V-shape gesture samples
    ├── Z/                      # Z-shape gesture samples
    └── some_class/             # Some other gesture samples
```

## 🚀 Setup Instructions

### 1. 📦 Install Dependencies

* Arduino IDE 2.3.2+
* Edge Impulse CLI
* Python 3.8+
* Flask: `pip install flask`
* Install required Arduino libraries:

  * `ArduinoJson`
  * `WiFi.h`
  * `Adafruit_MPU6050`
  * `HTTPClient`

### 2. 🔧 Hardware Setup

* ESP32 Xiao C3
* MPU6050 connected via I2C

### 3. 🌐 Update WiFi + Server Info

In `wand_offloading.ino`:

```cpp
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* serverUrl = "http://<your_server_ip>:8000/";
```

### 4. 📤 Upload Sketch to ESP32

* Use correct COM port
* Select board: `XIAO_ESP32C3`

### 5. 🧠 Run Cloud Inference Server

```bash
python app/app.py
```

Then open `http://127.0.0.1:8000` or your local IP.

---

## 💬 Discussion Questions

### 1. Is server’s confidence always higher than wand’s?

**Not always.** When the ESP32 detects a very clear gesture (like a strong "O"), its confidence can match or even exceed the cloud model. But in most noisy/uncertain situations, the cloud model performs better because it is larger, more accurate, and trained with more data.

### 2. Data Flow Diagram

```
[ESP32 + MPU6050] --raw features--> [Local Model Prediction] --confidence check-->
|--(high)--> Local classification
|--(low) --> WiFi --> HTTP POST --> [Flask Cloud Server] --> Model inference --> result
```

### 3. Edge-first, fallback-to-server Strategy

#### Connectivity

* ✅ Pros: Works even without WiFi if confident
* ❌ Cons: Cloud fallback needs internet

#### Latency

* ✅ Pros: Fast response from local model
* ❌ Cons: Delay for cloud inference

#### Prediction Consistency

* ✅ Pros: Mostly stable predictions
* ❌ Cons: Local vs. cloud results might differ

#### Data Privacy

* ✅ Pros: Keeps data local if confident
* ❌ Cons: Uncertain cases send raw features to server

### 4. Mitigation Strategy

Store fallback data locally during offline use and reattempt cloud upload once WiFi reconnects (buffer + retry).

---

## 🖥 Screenshot Proof

* ✅ Edge inference shown on Serial Monitor
* ✅ Cloud fallback shown via Flask terminal

---

## ✅ Rubric Checklist

*

Total: 25 pts

---

## 🔥 Clean Up Resources

If you deployed services to cloud (e.g. Azure/AWS), go to your dashboard and delete unnecessary **resource groups** to prevent unexpected charges.

---

Thanks for reviewing Lab 5! 🎉
