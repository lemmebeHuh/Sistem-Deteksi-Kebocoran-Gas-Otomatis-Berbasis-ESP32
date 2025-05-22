#include <WiFi.h>
#include <LiquidCrystal_I2C.h>
#include <WebServer.h>
#include <ESP32Servo.h>

// WiFi Credentials
const char* ssid = "Ilman";
const char* password = "123456789";

// Pin setup
#define MQ2_SENSOR_PIN 34
#define BUZZER_PIN 4
#define RED_LED_PIN 19
#define GREEN_LED_PIN 23
#define YELLOW_LED_PIN 18
#define WHITE_LED_PIN 27
#define SERVO_PIN 23
//#define MOTOR_IN1 12  // Motor DC IN1 (L293D)
//#define MOTOR_IN2 13  // Motor DC IN2 (L293D)

#define ADC_REF 3.3
#define ADC_RES 4095

LiquidCrystal_I2C lcd(0x27, 16, 2);
WebServer server(80);
Servo valveServo;

float ppm = 0.0;
String gasStatus = "AMAN";
bool whiteLampOn = true;

void setup() {
  Serial.begin(115200);

  // Pin Mode
  pinMode(MQ2_SENSOR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(WHITE_LED_PIN, OUTPUT);
  // pinMode(MOTOR_IN1, OUTPUT);
  // pinMode(MOTOR_IN2, OUTPUT);

  // Initial States
  digitalWrite(WHITE_LED_PIN, HIGH); // Lampu putih selalu nyala
  // digitalWrite(MOTOR_IN1, LOW);
  // digitalWrite(MOTOR_IN2, LOW);

  valveServo.attach(SERVO_PIN);
  valveServo.write(0); // Servo tertutup di awal

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected.");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi Terhubung");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());

  setupWebServer();
}

void setupWebServer() {
  server.on("/", HTTP_GET, []() {
    String color = gasStatus == "AMAN" ? "#2ecc71" : (gasStatus == "WASPADA" ? "#f1c40f" : "#e74c3c");

    String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1.0'>"
                  "<style>"
                  "body{font-family:sans-serif;margin:0;padding:0;display:flex;justify-content:center;align-items:center;height:100vh;background:" + color + ";}"
                  ".card{background:#fff;border-radius:20px;padding:30px;width:300px;box-shadow:0 10px 20px rgba(0,0,0,0.2);text-align:center;}"
                  ".status{font-size:20px;font-weight:bold;margin-bottom:20px;}"
                  ".gauge{width:200px;height:100px;background:conic-gradient(" + color + " 0deg 180deg, #ddd 180deg 360deg);border-radius:100px 100px 0 0;margin:0 auto;position:relative;}"
                  ".needle{width:6px;height:90px;background:#333;position:absolute;bottom:10px;left:50%;transform-origin:bottom center;transform:rotate(" + String(map(ppm, 0, 1000, -90, 90)) + "deg);border-radius:3px;}"
                  ".ppm{font-size:24px;margin-top:20px;font-weight:bold;}"
                  ".btn{margin-top:15px;padding:10px 20px;font-size:16px;background-color:#2c3e50;color:white;border:none;border-radius:10px;cursor:pointer;}"
                  "</style></head><body>"
                  "<div class='card'>"
                  "<div class='status'>Status Gas: " + gasStatus + "</div>"
                  "<div class='gauge'><div class='needle'></div></div>"
                  "<div class='ppm'>PPM: " + String((int)ppm) + "</div>"
                  "<form method='POST' action='/lampuoff'><button class='btn'>Matikan Listrik</button></form>"
                  "<form method='POST' action='/lampuon'><button class='btn'>Nyalakan Listrik</button></form>"
                  "</div></body></html>";

    server.send(200, "text/html", html);
  });

  server.on("/lampuoff", HTTP_POST, []() {
    whiteLampOn = false;
    digitalWrite(WHITE_LED_PIN, LOW);
    server.sendHeader("Location", "/");
    server.send(303);
  });

  server.on("/lampuon", HTTP_POST, []() {
    whiteLampOn = true;
    digitalWrite(WHITE_LED_PIN, HIGH);
    server.sendHeader("Location", "/");
    server.send(303);
  });

  server.begin();
}

float convertToPPM(int sensorValue) {
  float voltage = sensorValue * (ADC_REF / ADC_RES);
  return voltage * 1000;
}

// Motor Control Functions
// void motorMaju() {
//   digitalWrite(MOTOR_IN1, HIGH);
//   digitalWrite(MOTOR_IN2, LOW);
// }

// void motorStop() {
//   digitalWrite(MOTOR_IN1, LOW);
//   digitalWrite(MOTOR_IN2, LOW);
// }

void loop() {
  server.handleClient();

  int sensorValue = analogRead(MQ2_SENSOR_PIN);
  ppm = convertToPPM(sensorValue);
  Serial.println("PPM: " + String(ppm));

  lcd.clear();
  lcd.setCursor(0, 0);

  if (whiteLampOn) {
    digitalWrite(WHITE_LED_PIN, HIGH);
  }

  if (ppm >= 700) {
    gasStatus = "GAS BOCOR!";
    digitalWrite(RED_LED_PIN, HIGH);
    digitalWrite(YELLOW_LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, HIGH);
    valveServo.write(90);      // Buka katup
    // motorMaju();               // Aktifkan kipas
    lcd.print("Gas Bocor!!");
  } else {
    gasStatus = "AMAN";
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(YELLOW_LED_PIN, HIGH);
    digitalWrite(BUZZER_PIN, LOW);
    valveServo.write(0);       // Tutup katup
    // motorStop();               // Matikan kipas
    lcd.print("Gas Aman");
  }

  lcd.setCursor(0, 1);
  lcd.print("PPM: " + String((int)ppm));

  delay(1000);
}
