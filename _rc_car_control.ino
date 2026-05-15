#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "Go";
const char* password = "12345678";

WebServer server(80);

// =======================
// MOTOR PINS (L298N)
// =======================
// Motor pins
#define IN1 18
#define IN2 19
#define IN3 22
#define IN4 23

// Enable pins
#define ENA 25
#define ENB 21

// =======================
// STATE CONTROL
// =======================
String currentState = "IDLE";
unsigned long actionStartTime = 0;
int step = 0;

// =======================
// MOTOR FUNCTIONS
// =======================
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

// =======================
// HANDLE REQUEST
// =======================
void handleData() {

  if (server.hasArg("label")) {

    String label = server.arg("label");
    Serial.println("Received: " + label);

    // Ignore if already busy
    if (currentState != "IDLE") {
      server.send(200, "text/plain", "BUSY");
      return;
    }

    if (label == "BIODEGRADABLE") {
      // No movement
      Serial.println("BIO - Stay in center");
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

  } else {
    server.send(400, "text/plain", "No label");
  }
}

// =======================
// SETUP
// =======================
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

  Serial.println("\nConnected!");
  Serial.println(WiFi.localIP());

  server.on("/update", handleData);
  server.begin();
}

// =======================
// LOOP
// =======================
void loop() {

  server.handleClient();

  unsigned long currentTime = millis();

  // =======================
  // NON-BIO SEQUENCE
  // =======================
  if (currentState == "NONBIO") {

    if (step == 1 && currentTime - actionStartTime >= 300) {
      stopMotor();
      step = 2;
      actionStartTime = currentTime;
    }

    else if (step == 2 && currentTime - actionStartTime >= 15000) {
      moveBackward();
      step = 3;
      actionStartTime = currentTime;
    }

    else if (step == 3 && currentTime - actionStartTime >= 300) {
      stopMotor();
      currentState = "IDLE";
      Serial.println("NON-BIO cycle done");
    }
  }

  // =======================
  // E-WASTE SEQUENCE
  // =======================
  if (currentState == "EWASTE") {

    if (step == 1 && currentTime - actionStartTime >= 300) {
      stopMotor();
      step = 2;
      actionStartTime = currentTime;
    }

    else if (step == 2 && currentTime - actionStartTime >= 15000) {
      moveForward();
      step = 3;
      actionStartTime = currentTime;
    }

    else if (step == 3 && currentTime - actionStartTime >= 300) {
      stopMotor();
      currentState = "IDLE";
      Serial.println("E-WASTE cycle done");
    }
  }
}