#include <WiFi.h>
#include <WebServer.h>
#include <HardwareSerial.h>
#include <WiFiClientSecure.h>

// ================= WIFI =================
const char* ssid = "madhan";
const char* password = "12345678";

WebServer server(80);

// ================= TELEGRAM =================
WiFiClientSecure client;

String BOT_TOKEN = "8737050326:AAFbMC70avg-KdPTDrdE6eCJqVfbPu6HMlw";   // 🔴 CHANGE
String CHAT_ID   = "5466757727";

//String BOT_TOKEN = "8747857976:AAFOJir4BifSQr6eGC9_DwFNC3VkBaq13uE";
//String CHAT_ID   = "1868919797";

// ================= GSM =================
HardwareSerial SIM800(2);

String senderNumber = "";
String SMCANO = "";
bool waitForMessage = false;

String alertNumber = "+919092300701";

// ================= ULTRASONIC =================
#define TRIG_PIN 23
#define ECHO_PIN 22

float EMPTY_DIST = 20.0;
float FULL_DIST  = 8.0;
float FULL_THRESHOLD = 85.0;

float currentLevel = 0;

// ================= STATE =================
String currentWaste = "NONE";
String lastSentWaste = "";
bool measuring = false;
bool alreadyMeasured = false;

unsigned long startTime = 0;
float maxLevel = 0;

// ======================================================
// ================= TELEGRAM FUNCTION ===================
// ======================================================
void sendTelegram(String message)
{
  client.setInsecure();

  for (int i = 0; i < 3; i++)
  {
    Serial.println("Connecting to Telegram...");

    if (client.connect("api.telegram.org", 443))
    {
      Serial.println("✅ Connected");

      String url = "/bot" + BOT_TOKEN + "/sendMessage?chat_id=" + CHAT_ID + "&text=" + message;

      client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                   "Host: api.telegram.org\r\n" +
                   "Connection: close\r\n\r\n");

      delay(300);

      String response = "";

      while (client.available()) {
        response += client.readString();
      }

      client.stop();

      // 🔥 ACK CHECK
      if (response.indexOf("\"ok\":true") != -1)
      {
        Serial.println("✅ TELEGRAM DELIVERED SUCCESSFULLY");
      }
      else
      {
        Serial.println("❌ TELEGRAM DELIVERY FAILED");
      }

      Serial.println("📩 Message: " + message);
      return;
    }

    Serial.println("❌ Retry...");
    delay(500);
  }

  Serial.println("❌ Telegram Failed After Retries");
}

// ================= ULTRASONIC =================

float getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  return duration * 0.034 / 2;
}

float getAverageDistance() {
  float sum = 0;
  for (int i = 0; i < 5; i++) {
    sum += getDistance();
    delay(10);
  }
  return sum / 5;
}

float getLevel(float d)
{
  if (d <= 0 || d > 50) return currentLevel;

  float level = ((EMPTY_DIST - d) / (EMPTY_DIST - FULL_DIST)) * 100.0;
  return constrain(level, 0, 100);
}

// ================= WEB =================
void handleRoot()
{
  String status = (currentLevel >= FULL_THRESHOLD) ? "FULL" : "NORMAL";

  String html = "<html><head>";
  html += "<meta http-equiv='refresh' content='2'>";
  html += "<style>body{font-family:Arial;text-align:center;}</style>";
  html += "</head><body>";

  html += "<h1>Smart Waste Bin Monitor</h1>";
  html += "<h2>Waste: " + currentWaste + "</h2>";
  html += "<h2>Level: " + String(currentLevel,1) + "%</h2>";
  html += "<h2>Status: " + status + "</h2>";

  html += "</body></html>";

  server.send(200, "text/html", html);
}

// ================= HANDLE RC =================
void handleUpdate()
{
  if (!server.hasArg("label")) {
    server.send(400, "text/plain", "No label");
    return;
  }

  currentWaste = server.arg("label");

  // 🔥 Reset measurement
  measuring = true;
  alreadyMeasured = false;
  startTime = millis();
  maxLevel = 0;

  // 🔥 Avoid duplicate detection spam
  if (currentWaste != lastSentWaste)
  {
    sendTelegram("Detected: " + currentWaste);
    delay(800);
    lastSentWaste = currentWaste;
  }

  server.send(200, "text/plain", "OK");
}

// ================= SETUP =================
void setup()
{
  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  SIM800.begin(9600, SERIAL_8N1, 16, 17);
  delay(3000);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("\nWiFi Connected!");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/update", handleUpdate);

  server.begin();
}

// ================= LOOP =================
void loop()
{
  server.handleClient();

  if (measuring && !alreadyMeasured)
  {
    float d = getAverageDistance();
    currentLevel = getLevel(d);

    if (currentLevel > maxLevel)
      maxLevel = currentLevel;

    if (millis() - startTime >= 10000)
    {
      Serial.print("Max Level Final: ");
      Serial.println(maxLevel);

      String msg;

      if (maxLevel >= FULL_THRESHOLD)
      {
        msg = "🚨 " + currentWaste + " BIN FULL (" + String(maxLevel,1) + "%)";
      }
      else
      {
        msg = "ℹ️ " + currentWaste + " Level: " + String(maxLevel,1) + "%";
      }

      sendTelegram(msg);

      alreadyMeasured = true;   // 🔥 prevent repeat
      measuring = false;
    }
  }
}