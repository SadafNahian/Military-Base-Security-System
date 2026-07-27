#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32_Servo.h>
#include <DHT.h>

// Pin definitions
#define FIRE_SENSOR_PIN 23
#define GAS_SENSOR_PIN 34
#define DHT_PIN 5
#define DHT_TYPE DHT11
#define SERVO_PIN 14
#define ULTRASONIC_TRIG_PIN 26
#define ULTRASONIC_ECHO_PIN 27
#define IR_SENSOR_PIN 19
#define GSM_RX_PIN 33
#define GSM_TX_PIN 25
#define BUZZER_PIN 4

// Components initialization
DHT dht(DHT_PIN, DHT_TYPE);
Servo servo;
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Variables
long duration;
float distance;
int fireSensorValue;
int gasSensorValue;
int irSensorState;

// Add a global string variable for the phone number
String phoneNumber = "01711424928";  // Replace with the recipient's phone number


void setup() {
  // Begin serial communication
  Serial.begin(9600);

  // GSM communication
  Serial2.begin(9600, SERIAL_8N1, GSM_RX_PIN, GSM_TX_PIN);
  Serial2.println("AT");  // Test GSM module

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
  lcd.clear();
}

void loop() {
  // Read fire sensor
  fireSensorValue = digitalRead(FIRE_SENSOR_PIN);
  Serial.print("Fire Sensor Value: ");
  Serial.println(fireSensorValue);

  // Read gas sensor
  gasSensorValue = analogRead(GAS_SENSOR_PIN);
  Serial.print("Gas Sensor Value: ");
  Serial.println(gasSensorValue);

  // Check for fire
  if (fireSensorValue == LOW) {
    lcd.clear();
    lcd.setCursor(0, 1);
    Serial.println("Fire Detected!");
    lcd.print("Fire Detected!");

    beepBuzzer(5);  // Beep the buzzer 5 times

    sendSMS("Fire Detected!");

    lcd.clear();
  }

  // gas detection
  if (gasSensorValue > 2000) {
    lcd.clear();
    lcd.setCursor(0, 1);
    Serial.println("Gas Detected!");
    lcd.print("Gas Detected!");

    beepBuzzer(5);  // Beep the buzzer 5 times
    sendSMS("Gas Detected");

    lcd.clear();
  }

  // Read DHT11 sensor
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  if (!isnan(temperature) && !isnan(humidity)) {
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(" C");
    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");

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
    Serial.println("DHT11 Error");
    lcd.setCursor(0, 0);
    lcd.print("DHT Error");
  }


  // IR sensor
  irSensorState = digitalRead(IR_SENSOR_PIN);
  Serial.print("IR Sensor State: ");
  Serial.println(irSensorState);

  // lcd.setCursor(0, 1);
  // if (irSensorState == HIGH) {
  //   lcd.clear();
  //   Serial.println("IR Detected Object!");
  //   lcd.print("IR: DETECT");
  // } else {
  //   // Serial.println("No Object Detected by IR.");
  //   // lcd.print("IR: NONE");
  // }


  delay(100);  // Main loop delay
}

void beepBuzzer(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(BUZZER_PIN, HIGH);  // Turn on the buzzer
    delay(200);                      // Beep duration
    digitalWrite(BUZZER_PIN, LOW);   // Turn off the buzzer
    delay(200);                      // Delay between beeps
  }
}

void sendSMS(String message) {
  Serial2.println("AT+CMGF=1");  // Set SMS mode to text
  delay(1000);
  Serial2.print("AT+CMGS=\"");
  Serial2.print(phoneNumber);  // Use the phone number variable
  Serial2.println("\"");
  delay(1000);
  Serial2.print(message);  // Send the message
  delay(1000);
  Serial2.write(26);  // Send Ctrl+Z to indicate the end of the message
  delay(1000);
}