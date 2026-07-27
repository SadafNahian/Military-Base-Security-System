#include <WiFi.h>
#include <HTTPClient.h>

// Replace with your WiFi credentials
const char* ssid = "iot";
const char* password = "12121212";

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include <DHT.h>

// Pin definitions
#define FIRE_SENSOR_PIN 23
#define GAS_SENSOR_PIN 34
#define DHT_PIN 5
#define DHT_TYPE DHT11
#define SERVO_PIN 14
#define ULTRASONIC_TRIG_PIN 27
#define ULTRASONIC_ECHO_PIN 26
#define IR_SENSOR_PIN 19
#define GSM_RX_PIN 33
#define GSM_TX_PIN 25
#define BUZZER_PIN 4

// Components initialization
DHT dht(DHT_PIN, DHT_TYPE);
Servo servo;
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Variables
int fireSensorValue, gasSensorValue, irSensorState;
//String phoneNumber = "01870801955"; 
String phoneNumber = "01711424928"; // Replace with the recipient's phone number
//String phoneNumber = "01956587584";

// Radar scanning variables
int angle = 0;
bool increasing = true;

long distance;

unsigned long previousMillis = 0;  // Store last time event ran
const long interval = 20000;       // 20 seconds

float temperature;
float humidity;

void setup() {
  Serial.begin(115200);

  // GSM communication
  Serial2.begin(9600, SERIAL_8N1, GSM_RX_PIN, GSM_TX_PIN);

  // Initialize sensors and peripherals
  pinMode(FIRE_SENSOR_PIN, INPUT);
  pinMode(GAS_SENSOR_PIN, INPUT);
  pinMode(IR_SENSOR_PIN, INPUT);
  pinMode(ULTRASONIC_TRIG_PIN, OUTPUT);
  pinMode(ULTRASONIC_ECHO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  dht.begin();
  servo.attach(SERVO_PIN);
  lcd.begin();
  lcd.backlight();

  // Display startup message
  lcd.setCursor(0, 0);
  lcd.print("System Starting");
  Serial.println("System Starting...");
  delay(2000);


  // Start FreeRTOS task for radar on Core 0
  xTaskCreatePinnedToCore(
    radarTask,  // Task function
    "Radar Task",
    4096,  // Stack size
    NULL,
    1,  // Priority
    NULL,
    0  // Core 0
  );

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  lcd.clear();
}

// **🔹 Main loop (Core 1) - Sensor Handling & Alerts**
void loop() {

  unsigned long currentMillis = millis();

  // Check if 20 seconds have passed
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;  // Update last execution time

    // Call your function here

    Serial.println("");
    Serial.println("Millis Function");
    Serial.println("");
  }


  fireSensorValue = digitalRead(FIRE_SENSOR_PIN);
  gasSensorValue = analogRead(GAS_SENSOR_PIN);
  irSensorState = digitalRead(IR_SENSOR_PIN);

  // Serial.print("Fire Sensor: ");
  // Serial.println(fireSensorValue);
  // Serial.print("Gas Sensor: ");
  // Serial.println(gasSensorValue);
  //Serial.print("IR Sensor: ");
  //Serial.println(irSensorState);


  // IR Sensor
  if (irSensorState == LOW) {
    lcd.clear();
    lcd.setCursor(0, 1);
    Serial.println(" Gate Approach! ");
    lcd.print("Gate Approach !");
    beepBuzzer(5);
    lcd.clear();

    // Call the function to send data
    sendDataToServer(String(temperature), String(humidity), "0", "", "", "Gate_Approach", "0");
  }

  // 🔥 Fire Detection
  if (fireSensorValue == LOW) {
    lcd.clear();
    lcd.setCursor(0, 1);
    Serial.println("🔥 Fire Detected!");
    lcd.print("Fire Detected!");
    beepBuzzer(5);
    sendSMS("Fire Detected!");
    lcd.clear();

    // Call the function to send data
    sendDataToServer(String(temperature), String(humidity), "0", "Fire_Detected", "", "", "0");
  }

  // 🛢 Gas Detection
  if (gasSensorValue > 2000) {
    lcd.clear();
    lcd.setCursor(0, 1);
    Serial.println("🛢 Gas Detected!");
    lcd.print("Gas Detected!");
    beepBuzzer(5);
    sendSMS("Gas Detected!");
    lcd.clear();

    // Call the function to send data
    sendDataToServer(String(temperature), String(humidity), "0", "", "Gas_Detected", "", "0");
  }

  // 🌡 Temperature & Humidity Display
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
  if (!isnan(temperature) && !isnan(humidity)) {
    // Serial.print("🌡 Temp: ");
    // Serial.print(temperature);
    // Serial.print(" C | Hum: ");
    // Serial.print(humidity);
    // Serial.println(" %");

    lcd.setCursor(0, 0);
    lcd.print("Temp: ");
    lcd.print(temperature);
    lcd.print(char(223));
    lcd.print("C");

    lcd.setCursor(0, 1);
    lcd.print("Hum: ");
    lcd.print(humidity);
    lcd.print("%");
  } else {
    Serial.println("❌ DHT11 Error");
    lcd.setCursor(0, 0);
    lcd.print("DHT Error");
  }

  delay(100);
}

// **🔹 Radar Task (Core 0) - Object Detection**
void radarTask(void* pvParameters) {
  while (1) {
    distance = getDistance();

    // 📍 Print real-time angle & distance
    Serial.print("Angle: ");
    Serial.print(angle);
    Serial.print("° | Distance: ");
    Serial.print(distance);
    Serial.println(" cm");

    if (distance > 15 || distance == 0) {  // No object detected
      //digitalWrite(BUZZER_PIN, LOW);

      // Move the servo smoothly
      if (increasing) {
        angle += 2;
        if (angle >= 180) increasing = false;
      } else {
        angle -= 2;
        if (angle <= 0) increasing = true;
      }

      servo.write(angle);
      vTaskDelay(50 / portTICK_PERIOD_MS);  // Smooth delay
    } else {                                // 🚨 Object Detected within 15 cm

      vTaskDelay(500 / portTICK_PERIOD_MS);

      distance = getDistance();

      if (distance > 15 || distance == 0) {

      } else {

        beepBuzzer(3);  // Beep 3 times

        Serial.print("🚨 ALERT! Object at Angle: ");
        Serial.print(angle);
        Serial.print("° | Distance: ");
        Serial.print(distance);
        Serial.println(" cm");

        String intru = "Intruder Detect! " + String(distance) + "cm " + String(angle) + "deg";

        sendSMS(intru);

        // Call the function to send data
        sendDataToServer(String(temperature), String(humidity), String(distance), "", "", "", String(angle));


        vTaskDelay(5000 / portTICK_PERIOD_MS);  // Stop servo for 1 sec

        lcd.clear();
      }
    }
  }
}

// **🔹 Get Distance from Ultrasonic Sensor**
long getDistance() {
  digitalWrite(ULTRASONIC_TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(ULTRASONIC_TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(ULTRASONIC_TRIG_PIN, LOW);

  long duration = pulseIn(ULTRASONIC_ECHO_PIN, HIGH);
  long distance = duration * 0.034 / 2;
  return distance;
}

// **🔹 Beep Buzzer Function**
void beepBuzzer(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(200);
    digitalWrite(BUZZER_PIN, LOW);
    delay(200);
  }
}

// **🔹 Send SMS via GSM**
void sendSMS(String message) {
  Serial2.println("AT+CMGF=1");
  delay(1000);
  Serial2.print("AT+CMGS=\"");
  Serial2.print(phoneNumber);
  Serial2.println("\"");
  delay(1000);
  Serial2.print(message);
  delay(1000);
  Serial2.write(26);
  delay(1000);
}

void sendDataToServer(String temperature, String humidity, String distance, String fire, String gas, String IR_sensor, String servo_angle) {
  HTTPClient http;



  // Prepare the URL with query parameters
  String url = "https://script.google.com/macros/s/AKfycbx13qXw_vbYSfd9kgO6YtDL162oYMQGfnTMCKIS7xOClGEJ9c2nJDVJUrbJKmLz5cky/exec?";
  url += "temperature=" + String(temperature);
  url += "&humidity=" + String(humidity);
  url += "&distance=" + String(distance);
  url += "&fire=" + String(fire);
  url += "&gas=" + String(gas);
  url += "&IR_sensor=" + String(IR_sensor);
  url += "&servo_angle=" + String(servo_angle);

  // Send the GET request
  http.begin(url);
  int httpCode = http.GET();  // Send GET request

  // Check the response code
  if (httpCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpCode);
    String payload = http.getString();  // Get response payload (if needed)
    Serial.println(payload);
  } else {
    Serial.print("HTTP Request failed: ");
    Serial.println(httpCode);
  }

  // End the HTTP request
  http.end();
}
