#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>

const char* ssid = "madhan";
const char* password = "12345678";

WebServer server(80);

// 🔴 CHANGE THIS (MAIN ESP32 IP)
String MAIN_ESP_URL = "http://10.48.248.38/update"; //esp32-637EF8

// ================= MOTOR PINS =================
#define IN1 18
#define IN2 19
#define IN3 22
#define IN4 23

#define ENA 25
#define ENB 21

// ================= STATE =================
String currentState = "IDLE";
unsigned long actionStartTime = 0;
int step = 0;
String currentLabel = "";

// ================= MOTOR =================
void stopMotor() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

void moveForward() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void moveBackward() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

// ================= SEND TO MAIN ESP =================
void sendToMainESP(String label) {
  HTTPClient http;

  String url = MAIN_ESP_URL + "?label=" + label;
  http.begin(url);

  int httpCode = http.GET();

  Serial.println("Sent to MAIN ESP: " + label);
  Serial.println("HTTP Code: " + String(httpCode));

  http.end();
}

// ================= HANDLE REQUEST =================
void handleData() {

  if (!server.hasArg("label")) {
    server.send(400, "text/plain", "No label");
    return;
  }

  String label = server.arg("label");
  Serial.println("Received: " + label);

  if (currentState != "IDLE") {
    server.send(200, "text/plain", "BUSY");
    return;
  }

  currentLabel = label;

  if (label == "BIODEGRADABLE") {
    Serial.println("BIO - No movement");
    sendToMainESP(currentLabel);
  }
  else if (label == "NON-BIODEGRADABLE") {
    currentState = "NONBIO";
    step = 1;
    actionStartTime = millis();
    moveForward();
  }
  else if (label == "E-WASTE") {
    currentState = "EWASTE";
    step = 1;
    actionStartTime = millis();
    moveBackward();
  }

  server.send(200, "text/plain", "OK");
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);

  digitalWrite(ENA, HIGH);
  digitalWrite(ENB, HIGH);

  stopMotor();

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nRC ESP Connected!");
  Serial.println(WiFi.localIP());

  server.on("/update", handleData);
  server.begin();
}

// ================= LOOP =================
void loop() {

  server.handleClient();
  unsigned long now = millis();

  // -------- NON BIO --------
  if (currentState == "NONBIO") {

    if (step == 1 && now - actionStartTime >= 300) {
      stopMotor();
      delay(100);
      step = 2;
      actionStartTime = now;

      sendToMainESP(currentLabel); // 🔥 SEND HERE
    }

    else if (step == 2 && now - actionStartTime >= 15000) {
      moveBackward();
      step = 3;
      actionStartTime = now;
    }

    else if (step == 3 && now - actionStartTime >= 300) {
      stopMotor();
      delay(100);
      currentState = "IDLE";
    }
  }

  // -------- E-WASTE --------
  if (currentState == "EWASTE") {

    if (step == 1 && now - actionStartTime >= 300) {
      stopMotor();
      delay(100);
      step = 2;
      actionStartTime = now;

      sendToMainESP(currentLabel); // 🔥 SEND HERE
    }

    else if (step == 2 && now - actionStartTime >= 15000) {
      moveForward();
      step = 3;
      actionStartTime = now;
    }

    else if (step == 3 && now - actionStartTime >= 300) {
      stopMotor();
      delay(100);
      currentState = "IDLE";
    }
  }
}