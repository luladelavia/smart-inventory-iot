#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <RTClib.h>

const char* ssid           = "Wokwi-GUEST";
const char* password       = "";
const char* mqtt_server    = "broker.hivemq.com";
const char* mqtt_id_prefix = "esp32-stok-dimas-";
const char* mqtt_topic     = "gudang/stok/rak1";

#define TRIG_PIN 5
#define ECHO_PIN 18
#define SDA_PIN  21
#define SCL_PIN  22

const float JARAK_PENUH  = 10.0;
const float JARAK_KOSONG = 30.0;
const unsigned long INTERVAL_MS = 2000;

WiFiClient espClient;
PubSubClient client(espClient);
RTC_DS1307 rtc;
unsigned long lastRead = 0;
String clientId;

void setup_wifi() {
  WiFi.begin(ssid, password, 6);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
}

void reconnect() {
  while (!client.connected()) {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi putus, menyambung ulang...");
      WiFi.reconnect();
      delay(1000);
      continue;
    }

    Serial.print("Connecting MQTT...");
    if (client.connect(clientId.c_str())) {
      Serial.println("OK");
    } else {
      Serial.printf("gagal (rc=%d), coba lagi 5 detik\n", client.state());
      delay(5000);
    }
  }
}

float bacaJarakCm() {
  digitalWrite(TRIG_PIN, LOW);  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long durasi = pulseIn(ECHO_PIN, HIGH, 30000);
  if (durasi == 0) return -1;
  return durasi * 0.0343 / 2.0;
}

float bacaJarakRata(int n = 5) {
  float total = 0;
  int valid = 0;
  for (int i = 0; i < n; i++) {
    float d = bacaJarakCm();
    if (d > 0) { total += d; valid++; }
    delay(30);
  }
  return valid ? total / valid : -1;
}

const char* statusStok(int persen) {
  if (persen >= 70) return "PENUH";
  if (persen >= 30) return "SEDANG";
  if (persen >= 10) return "MENIPIS";
  return "HABIS";
}

void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  Wire.begin(SDA_PIN, SCL_PIN);
  if (!rtc.begin()) {
    Serial.println("RTC tidak terdeteksi!");
    while (true) delay(1000);
  }
  if (!rtc.isrunning()) rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  setup_wifi();

  clientId = String(mqtt_id_prefix) + String((uint32_t)esp_random(), HEX);
  Serial.println("Client ID: " + clientId);

  client.setServer(mqtt_server, 1883);
  client.setBufferSize(512);
  client.setKeepAlive(30);
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  if (millis() - lastRead < INTERVAL_MS) return;
  lastRead = millis();

  float jarak = bacaJarakRata();
  if (jarak < 0) {
    Serial.println("Sensor tidak membaca");
    return;
  }

  float persen = (JARAK_KOSONG - jarak) / (JARAK_KOSONG - JARAK_PENUH) * 100.0;
  persen = constrain(persen, 0, 100);

  DateTime now = rtc.now();
  char ts[24];
  snprintf(ts, sizeof(ts), "%04d-%02d-%02dT%02d:%02d:%02d",
           now.year(), now.month(), now.day(),
           now.hour(), now.minute(), now.second());

  char payload[400];
  snprintf(payload, sizeof(payload),
    "{\"device\":\"rak1\","
    "\"status\":\"%s\","
    "\"data\":["
      "{\"name\":\"stok\",\"value\":%d,\"unit\":\"%%\",\"timestamp\":\"%s\"},"
      "{\"name\":\"jarak\",\"value\":%.1f,\"unit\":\"cm\",\"timestamp\":\"%s\"}"
    "]}",
    statusStok((int)persen), (int)persen, ts, jarak, ts);

  bool ok = client.publish(mqtt_topic, payload);
  Serial.printf("%s %s\n", ok ? "TERKIRIM" : "GAGAL", payload);
}