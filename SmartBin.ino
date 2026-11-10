#include <WiFi.h>
#include "esp_camera.h"
#include <ESP32Servo.h>
#include "edge-impulse-sdk/classifier/ei_run_classifier.h"

// ==== WiFi ====
const char* ssid = "NAMA_WIFI";
const char* password = "PASSWORD_WIFI";

// ==== IR Sensor ====
const int IR_PIN = 33; // pin IR sensor OUT

// ==== Servo Pemilah ====
Servo servoPemilah;
const int SERVO_PIN = 14;
const int POS_TENGAH = 90;
const int POS_BOTOL = 135;
const int POS_KRESEK = 45;

// ==== Ultrasonik ====
const int TRIG_BOTOL = 12;
const int ECHO_BOTOL = 13;
const int TRIG_KRESEK = 15;
const int ECHO_KRESEK = 2;

long lastDistBotol = 0;
long lastDistKresek = 0;
const float MAX_HEIGHT = 50.0; // cm

// ==== Kamera ====
#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

WiFiServer server(80);
camera_fb_t *fb = NULL;

// baca ultrasonic
long readUltrasonic(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH, 30000);
  long distance = duration * 0.034 / 2;
  return distance;
}

void setup() {
  Serial.begin(115200);

  // WiFi
  WiFi.begin(ssid, password);
  Serial.print("Menghubungkan WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\nWiFi terhubung");
  Serial.print("IP Address: "); Serial.println(WiFi.localIP());

  server.begin();

  // Servo pemilah
  servoPemilah.attach(SERVO_PIN);
  servoPemilah.write(POS_TENGAH);

  // Ultrasonik
  pinMode(TRIG_BOTOL, OUTPUT); pinMode(ECHO_BOTOL, INPUT);
  pinMode(TRIG_KRESEK, OUTPUT); pinMode(ECHO_KRESEK, INPUT);

  // IR sensor
  pinMode(IR_PIN, INPUT);

  // Konfigurasi kamera
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0       = Y2_GPIO_NUM;
  config.pin_d1       = Y3_GPIO_NUM;
  config.pin_d2       = Y4_GPIO_NUM;
  config.pin_d3       = Y5_GPIO_NUM;
  config.pin_d4       = Y6_GPIO_NUM;
  config.pin_d5       = Y7_GPIO_NUM;
  config.pin_d6       = Y8_GPIO_NUM;
  config.pin_d7       = Y9_GPIO_NUM;
  config.pin_xclk     = XCLK_GPIO_NUM;
  config.pin_pclk     = PCLK_GPIO_NUM;
  config.pin_vsync    = VSYNC_GPIO_NUM;
  config.pin_href     = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn     = PWDN_GPIO_NUM;
  config.pin_reset    = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_RGB565; 
  config.frame_size   = FRAMESIZE_QVGA;
  config.jpeg_quality = 12;
  config.fb_count     = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Kamera gagal diinisialisasi, kode error 0x%x", err);
    return;
  }
  Serial.println("Kamera siap.");
}

void loop() {
  int irState = digitalRead(IR_PIN);

  // hanya proses klasifikasi kalau IR mendeteksi sampah
  if (irState == LOW) { // LOW = objek terdeteksi
    Serial.println("🚮 Sampah terdeteksi oleh IR, memulai klasifikasi kamera...");

    delay(500); // beri jeda 0.5 detik agar pintu/gerakan tidak terbaca IR

    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Gagal ambil frame kamera");
      return;
    }

    ei::signal_t signal;
    // ei::image::signal_from_rgb565(fb->buf, fb->width, fb->height, &signal);

    ei_impulse_result_t result = { 0 };
    EI_IMPULSE_ERROR ei_error = run_classifier(&signal, &result, false);
    if (ei_error != EI_IMPULSE_OK) {
      Serial.printf("Classifier error: %d\n", ei_error);
      esp_camera_fb_return(fb);
      return;
    }

    // hasil prediksi
    float max_val = 0;
    String best_label = "";
    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
      if (result.classification[ix].value > max_val) {
        max_val = result.classification[ix].value;
        best_label = result.classification[ix].label;
      }
    }

    Serial.print("Kategori: ");
    Serial.print(best_label);
    Serial.print(" (");
    Serial.print(max_val * 100, 2);
    Serial.println("%)");

    // Gerakkan servo sesuai hasil klasifikasi
    if (best_label == "Botol") {
      servoPemilah.write(POS_BOTOL);
      Serial.println("Servo ke posisi BOTOL");
    } else if (best_label == "Kresek") {
      servoPemilah.write(POS_KRESEK);
      Serial.println("Servo ke posisi KRESEK");
    } else {
      servoPemilah.write(POS_TENGAH);
      Serial.println("Servo posisi tengah (Unknown)");
    }

    delay(1000); // beri waktu sampah jatuh
    servoPemilah.write(POS_TENGAH);

    // baca kapasitas bak
    lastDistBotol = readUltrasonic(TRIG_BOTOL, ECHO_BOTOL);
    lastDistKresek = readUltrasonic(TRIG_KRESEK, ECHO_KRESEK);
    float percentBotol = (MAX_HEIGHT - lastDistBotol) / MAX_HEIGHT * 100;
    float percentKresek = (MAX_HEIGHT - lastDistKresek) / MAX_HEIGHT * 100;

    Serial.print("Kapasitas Botol: "); Serial.print(percentBotol); Serial.println("%");
    Serial.print("Kapasitas Kresek: "); Serial.print(percentKresek); Serial.println("%");

    esp_camera_fb_return(fb);
  }

  delay(100);
}