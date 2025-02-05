#include <SPI.h>
#include <mcp2515.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>

// DHT Sensor Configuration
#define DHTPIN 8              // DHT11 connected to digital pin 8
#define DHTTYPE DHT11         // DHT11 sensor type
DHT dht(DHTPIN, DHTTYPE);     // Initialize DHT11 sensor

// LCD Configuration
LiquidCrystal_I2C lcd(0x27, 16, 2);

MCP2515 mcp2515(10);          // MCP2515 CS pin

int currentDistance = -1;  // To store the latest distance value

void setup() {
  Serial.begin(9600);
  dht.begin();                // Initialize DHT sensor
  SPI.begin();                // Initialize SPI communication

  // Initialize MCP2515
  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
  mcp2515.setNormalMode();

  // Initialize LCD
  lcd.begin();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Initializing...");
  delay(2000);
  lcd.clear();

  Serial.println("DHT11 Transmitter Ready");
}

void loop() {
  Serial.println("Reading DHT11...");
  float actualTemperature = dht.readTemperature();  // Read current temperature
  int humidity = dht.readHumidity();               // Read humidity

  // Validate sensor readings
  if (isnan(humidity) || isnan(actualTemperature)) {
    Serial.println("Failed to read from DHT sensor!");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Sensor Error!");
    delay(2000); // Longer delay to allow sensor recovery
    return;
  }

  // Read Potentiometer for Temperature Adjustment
  int potValue = analogRead(A0);
  float adjustedTemperature = map(potValue, 0, 1023, actualTemperature - 10, actualTemperature + 10);

  // Check for new CAN messages
  struct can_frame canMsg;
  if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
    if (canMsg.can_id == 0x100) { // Ultrasonic Sensor (Distance)
      currentDistance = canMsg.data[0];
      Serial.print("Distance Received: ");
      Serial.print(currentDistance);
      Serial.println(" cm");
    }
  }

  // Update LCD display
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(actualTemperature, 1);
  lcd.print(" C");

  lcd.setCursor(0, 1);
  lcd.print("Adj Temp: ");
  lcd.print(adjustedTemperature, 1);
  lcd.print(" C");

  // If distance < 30 cm, display "Impact!"
  if (currentDistance >= 0 && currentDistance < 30) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Impact!");
    lcd.setCursor(0, 1);
    lcd.print("Dist: ");
    lcd.print(currentDistance);
    lcd.print(" cm");
  } else {
    lcd.setCursor(0, 1);
    lcd.print("Humi: ");
    lcd.print(humidity);
    lcd.print(" %");
  }

  // Debugging Output
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.print(" %, Actual Temp: ");
  Serial.print(actualTemperature);
  Serial.print(" C, Adjusted Temp: ");
  Serial.print(adjustedTemperature);
  Serial.println(" C");

  // Prepare CAN message
  canMsg.can_id  = 0x200;        // CAN ID for DHT11 sensor data
  canMsg.can_dlc = 2;            // Data length: 2 bytes
  canMsg.data[0] = humidity;     // Store humidity in first byte
  canMsg.data[1] = (int)adjustedTemperature;  // Store adjusted temperature in second byte

  // Send CAN message
  mcp2515.sendMessage(&canMsg);

  delay(2000); // Wait 2 seconds before next reading
}
