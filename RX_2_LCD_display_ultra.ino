#include <SPI.h>
#include <mcp2515.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2); // I2C LCD address
MCP2515 mcp2515(10);                // MCP2515 CS pin
struct can_frame canMsg;

int currentDistance = -1;  // To store the latest distance value
int currentHumidity = 0;   // To store the latest humidity value
int currentTemperature = 0; // To store the latest temperature value
unsigned long lastUpdate = 0; // Timer for LCD updates
const int updateInterval = 500; // Update LCD every 500ms

void setup() {
  Serial.begin(9600);
  SPI.begin();

  // Initialize LCD
  lcd.begin();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("CAN Reader");
  delay(2000); // Display initial message for 2 seconds
  lcd.clear();

  // Initialize MCP2515
  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
  mcp2515.setNormalMode();

  Serial.println("Receiver Ready");
}

void loop() {
  // Check for new CAN messages
  if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
    if (canMsg.can_id == 0x100) { // Ultrasonic Sensor (Distance)
      currentDistance = canMsg.data[0];
      Serial.print("Distance Received: ");
      Serial.print(currentDistance);
      Serial.println(" cm");
    } else if (canMsg.can_id == 0x200) { // DHT11 Sensor (Temperature & Humidity)
      currentHumidity = canMsg.data[0];
      currentTemperature = canMsg.data[1];
      Serial.print("Temperature Received: ");
      Serial.print(currentTemperature);
      Serial.print(" C, Humidity Received: ");
      Serial.print(currentHumidity);
      Serial.println(" %");
    }
  }

  // Update the LCD based on the latest values
  if (millis() - lastUpdate >= updateInterval) {
    lastUpdate = millis(); // Reset timer

    // Check if distance is below the threshold
    if (currentDistance >= 0 && currentDistance < 30) { // Valid distance below threshold
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Impact!");
      lcd.setCursor(0, 1);
      lcd.print("Distance: ");
      lcd.print(currentDistance);
      lcd.print(" cm");
    } else { // Default display: Temperature & Humidity
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Temp: ");
      lcd.print(currentTemperature);
      lcd.print(" C");

      lcd.setCursor(0, 1);
      lcd.print("Humi: ");
      lcd.print(currentHumidity);
      lcd.print(" %");
    }
  }
}
