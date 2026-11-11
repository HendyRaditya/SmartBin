#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <LittleFS.h>
#include <time.h>  // For real-time timestamp

// ---------------- WIFI CONFIG ----------------
const char* ssid = "jambi";
const char* password = "hendy2009";

// ---------------- NTP CONFIG (TIME) ----------------
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 8 * 3600;  // GMT+8
const int daylightOffset_sec = 0;

// ---------------- LCD INIT ----------------
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ---------------- SENSOR & SERVO PINS ----------------
const int IR_PIN = 26;
const int SERVO_MOUTH_PIN = 27;
const int SERVO_SORTER_PIN = 25;
const int TRIG_ORGANIC = 32;
const int ECHO_ORGANIC = 33;
const int TRIG_NONORGANIC = 14;
const int ECHO_NONORGANIC = 12;

// ---------------- SERVO POSITIONS ----------------
Servo servoMouth, servoSorter;
const int POS_CENTER = 90;
const int POS_ORGANIC = 20;
const int POS_NONORGANIC = 160;

// ---------------- WEBSERVER ----------------
WebServer server(80);

// ---------------- DATA VARIABLES ----------------
String lastTime = "None";
String wasteType = "-";
int organicCapacity = 0;
int inorganicCapacity = 0;

// ---------------- TIME FUNCTION ----------------
String getTimestamp() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "NTP Failed";
  }
  char timeString[30];
  strftime(timeString, sizeof(timeString), "%d-%m-%Y %H:%M:%S", &timeinfo);
  return String(timeString);
}

// ---------------- ULTRASONIC FUNCTION ----------------
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

int calcCapacity(long dist) {
  if (dist <= 0) return 0;
  if (dist >= 25) return 0;
  int cap = map(dist, 25, 0, 0, 100);
  if (cap < 0) cap = 0;
  if (cap > 100) cap = 100;
  return cap;
}

// ---------------- WEB HANDLERS ----------------
void handleRoot() {
  File file = LittleFS.open("/index.html", "r");
  server.streamFile(file, "text/html");
  file.close();
}

void handleData() {
  String json = "{";
  json += "\"time\":\"" + lastTime + "\",";
  json += "\"type\":\"" + wasteType + "\",";
  json += "\"organic_capacity\":" + String(organicCapacity) + ",";
  json += "\"inorganic_capacity\":" + String(inorganicCapacity);
  json += "}";
  server.send(200, "application/json", json);
}

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);

  // LCD setup
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Insert trash");

  // WiFi setup
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // NTP time setup
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println("NTP time synchronized.");

  // Filesystem setup
  if (!LittleFS.begin(true)) {
    Serial.println("Failed to mount LittleFS!");
    return;
  }

  // Web server routes
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
  Serial.println("Web server running.");

  // Initialize sensors & servos
  pinMode(IR_PIN, INPUT);
  pinMode(TRIG_ORGANIC, OUTPUT);
  pinMode(ECHO_ORGANIC, INPUT);
  pinMode(TRIG_NONORGANIC, OUTPUT);
  pinMode(ECHO_NONORGANIC, INPUT);

  servoMouth.attach(SERVO_MOUTH_PIN);
  servoSorter.attach(SERVO_SORTER_PIN);
  servoSorter.write(POS_CENTER);
}

// ---------------- LOOP ----------------
void loop() {
  server.handleClient();

  if (digitalRead(IR_PIN) == LOW) {
    Serial.println("Waste detected");

    lcd.clear();
    lcd.print("Classifying...");
    delay(3000);


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
    
    if (currentType == 1) {
      wasteType = "Organic";
      lcd.clear();
      lcd.print("Waste: Organic");
      delay(2000);
      lcd.clear();
      lcd.print("Processing...");
      servoSorter.write(POS_ORGANIC);
    } else {
      wasteType = "Non-Organic";
      lcd.clear();
      lcd.print("Waste: Non-Org");
      delay(2000);
      lcd.clear();
      lcd.print("Processing...");
      servoSorter.write(POS_NONORGANIC);
    }

    delay(2000);
    servoSorter.write(POS_CENTER);
    delay(1000);

    // Update kapasitas
    long distOrg = readUltrasonic(TRIG_ORGANIC, ECHO_ORGANIC);
    long distNon = readUltrasonic(TRIG_NONORGANIC, ECHO_NONORGANIC);
    organicCapacity = calcCapacity(distOrg);
    inorganicCapacity = calcCapacity(distNon);

    lastTime = getTimestamp();

    lcd.clear();
    lcd.print("Org:");
    lcd.print(organicCapacity);
    lcd.print("% Non:");
    lcd.print(inorganicCapacity);
    lcd.print("%");
    delay(3000);

    lcd.clear();
    lcd.print("Thank you!");
    delay(2000);

    lcd.clear();
    lcd.print("Insert trash");

    // Geser ke urutan berikutnya
    sequenceIndex++;
    if (sequenceIndex >= 6) sequenceIndex = 0;
  }
}
