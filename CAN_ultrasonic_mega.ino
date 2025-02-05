#include <SPI.h>
#include <mcp2515.h>

MCP2515 mcp2515(10); // CS pin for MCP2515
const int trigPin = 3; // Ultrasonic Trigger Pin
const int echoPin = 4; // Ultrasonic Echo Pin
const int buzzerPin = 9; // Buzzer Pin
const int collisionThreshold = 30; // Collision detection threshold in cm
const int validReadingMin = 2;    // Minimum valid distance in cm
const int validReadingMax = 400;  // Maximum valid distance in cm

void setup() {
  Serial.begin(9600);
  SPI.begin();

  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
  mcp2515.setNormalMode();

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(buzzerPin, OUTPUT);

  // Ensure the buzzer starts OFF
  digitalWrite(buzzerPin, LOW);
  Serial.println("Ultrasonic ECU Ready");
}

void loop() {
  long duration;
  float distance;

  // Trigger ultrasonic pulse
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Measure echo duration (timeout after 20ms if no echo is received)
  duration = pulseIn(echoPin, HIGH, 20000); // Wait for echo signal
  if (duration == 0) {
    Serial.println("No echo received. Skipping measurement.");
    turnOffBuzzer();
    return;
  }

  // Calculate distance in cm
  distance = duration * 0.034 / 2;

  // Validate the distance reading
  if (distance < validReadingMin || distance > validReadingMax) {
    Serial.println("Invalid distance reading. Skipping measurement.");
    turnOffBuzzer();
    return;
  }

  // Debugging distance
  Serial.print("Measured Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  // Check the threshold and control the buzzer
  if (distance < collisionThreshold) {
    Serial.println("Distance below threshold. Turning buzzer ON.");
    turnOnBuzzer(); // Turn the buzzer ON if below threshold
  } else {
    Serial.println("Distance above threshold. Turning buzzer OFF.");
    turnOffBuzzer(); // Turn the buzzer OFF if above threshold
  }

  // Send valid distance over CAN
  sendCANMessage(distance);

  delay(500); // Wait before the next measurement
}

void turnOnBuzzer() {
  digitalWrite(buzzerPin, HIGH);
}

void turnOffBuzzer() {
  digitalWrite(buzzerPin, LOW);
}

void sendCANMessage(float distance) {
  struct can_frame canMsg;
  canMsg.can_id = 0x100; // High-priority ID
  canMsg.can_dlc = 1;    // Data length
  canMsg.data[0] = (distance > 255) ? 255 : (int)distance; // Limit to 255 for single byte

  mcp2515.sendMessage(&canMsg);
  Serial.print("Distance Sent via CAN: ");
  Serial.println((int)distance);
}
