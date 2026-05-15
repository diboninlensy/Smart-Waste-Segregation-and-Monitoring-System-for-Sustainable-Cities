#include <WiFi.h>
#include <WebServer.h>

WebServer server(80);

// Motor pins
#define IN1 18
#define IN2 19
#define IN3 22
#define IN4 23

// Enable pins
#define ENA 25
#define ENB 21

// WiFi
const char* ssid = "Go";
const char* password = "12345678";


// -------- Motor Functions --------

void stopMotor()
{
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

void forward()
{
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);

  delay(200);
  stopMotor();
}

void backward()
{
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);

  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);

  delay(300);
  stopMotor();
}

void left()
{
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);

  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void right()
{
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}


// -------- Web Page --------

void handleRoot()
{
  String html = "<html><body style='text-align:center'>";
  html += "<h2>ESP32 RC CAR</h2>";

  html += "<a href='/forward'><button>Forward</button></a><br><br>";

  html += "<a href='/left'><button>Left</button></a>";
  html += "<a href='/stop'><button>Stop</button></a>";
  html += "<a href='/right'><button>Right</button></a><br><br>";

  html += "<a href='/back'><button>Backward</button></a>";

  html += "</body></html>";

  server.send(200, "text/html", html);
}


// -------- Setup --------

void setup()
{
  Serial.begin(115200);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);

  // Enable motors
  digitalWrite(ENA, HIGH);
  digitalWrite(ENB, HIGH);

  stopMotor();

  WiFi.begin(ssid, password);

  Serial.print("Connecting");

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("Connected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/forward", [](){ forward(); handleRoot(); });
  server.on("/back", [](){ backward(); handleRoot(); });
  server.on("/left", [](){ left(); handleRoot(); });
  server.on("/right", [](){ right(); handleRoot(); });
  server.on("/stop", [](){ stopMotor(); handleRoot(); });

  server.begin();
}


// -------- Loop --------

void loop()
{
  server.handleClient();
}