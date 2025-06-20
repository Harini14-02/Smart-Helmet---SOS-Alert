#include <Wire.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>

SoftwareSerial sim800(7, 8); // RX, TX for SIM800
SoftwareSerial gpsSerial(2, 3); // RX, TX for GPS

TinyGPSPlus gps;

bool callActive = false;
String phoneNumber = "+918898007594";

const int MPU = 0x68;  // MPU-6050 I2C address
int16_t AcX, AcY, AcZ, GyX, GyY, GyZ;  // Variables for accelerometer and gyroscope data

const int buzzerPin = 4;  // Buzzer pin
const int buttonPin = 12;  // Button pin
const int threshold = 18000;  // Threshold for acceleration (modify as needed)
unsigned long buzzerStartTime = 0;  // To track when buzzer started
bool buzzerActive = false;  // Buzzer state


void setup() {
  Wire.begin();
  Wire.beginTransmission(MPU);
  Wire.write(0x6B);  // Power management register
  Wire.write(0);     
  Wire.endTransmission(true);

  pinMode(buzzerPin, OUTPUT);  // Set buzzer pin as output
  pinMode(buttonPin, INPUT_PULLUP);  

  sim800.begin(9600);
  delay(2000);  // Allow SIM800 to initialize

  Serial.begin(9600);  // Start serial communication at 9600 baud
  gpsSerial.begin(9600); // Start GPS communication
  Serial.println("Waiting for GPS data...");
}

void makeCall() {
  Serial.println("Dialing the emergency contact...");
  sim800.println("ATD" + phoneNumber + ";"); // Dial the number
  delay(1000);
  
  callActive = true; // Set call active immediately

  unsigned long callStartTime = millis();
  bool callEnded = false;

  // Wait for the call to connect and check status
  while (millis() - callStartTime < 20000 && callActive) {
    sim800.println("AT+CLCC"); // Check active calls
    delay(1000);

    if (sim800.available()) {
      String callStatus = sim800.readString();
      Serial.println("Call Status: " + callStatus);

      // Check for active calls
      if (callStatus.indexOf("0,") != -1) { 
        Serial.println("Call is active.");
      } else {
        callEnded = true;
        break; 
      }
    }
  }

  if (!callEnded) {
    sim800.println("ATH");  // Hang up the call if not ended already
    delay(2000);  // Increase delay for reliability
  }
  Serial.println("Call ended or hung up.");
  sendLocationSMS(phoneNumber);
  callActive = false; // Mark call as inactive
}

/*****************************************************************************************
 * Function to send GPS location via SMS
*****************************************************************************************/
void sendLocationSMS(String number) {
  double latitude = gps.location.lat();
  double longitude = gps.location.lng();
  String mapLink = "https://www.google.com/maps?q=" + String(latitude, 6) + "," + String(longitude, 6);
  String message = "Alert: The call was disconnected. Here is the location: " + mapLink;
  sendSMS(number, message);
}

/*****************************************************************************************
 * Function to send SMS using SIM800
*****************************************************************************************/
void sendSMS(String number, String message) {
  sim800.println("AT+CMGF=1");  // Set SMS mode to text
  delay(500);
  
  sim800.println("AT+CMGS=\"" + number + "\"");  // Specify recipient number
  delay(500);
  
  sim800.println(message);  // Send the message
  delay(500);

  sim800.write(26);  // Send Ctrl+Z to submit the SMS
  delay(2000);  // Wait for SMS to be sent
  
  if (sim800.available()) {
    Serial.println(sim800.readString());  // Print response
  }

  Serial.println("SMS Sent!");
}

void loop() {
  Wire.beginTransmission(MPU);
  Wire.write(0x3B);  // Starting register for accelerometer data
  Wire.endTransmission(false);
  Wire.requestFrom(MPU, 14, true);  // Request 14 bytes of data

  // Reading accelerometer and gyroscope data
  AcX = Wire.read() << 8 | Wire.read();  // Accelerometer X-axis
  AcY = Wire.read() << 8 | Wire.read();  // Accelerometer Y-axis
  AcZ = Wire.read() << 8 | Wire.read();  // Accelerometer Z-axis
  GyX = Wire.read() << 8 | Wire.read();  // Gyroscope X-axis
  GyY = Wire.read() << 8 | Wire.read();  // Gyroscope Y-axis
  GyZ = Wire.read() << 8 | Wire.read();  // Gyroscope Z-axis

  // Displaying data on serial monitor
  Serial.print("AcX = "); Serial.print(AcX);
  Serial.print(" | AcY = "); Serial.print(AcY);
  Serial.print(" | AcZ = "); Serial.print(AcZ);
  Serial.print(" | GyX = "); Serial.print(GyX);
  Serial.print(" | GyY = "); Serial.print(GyY);
  Serial.print(" | GyZ = "); Serial.println(GyZ);

  // Check if any accelerometer value exceeds the threshold
  if ((abs(AcX) > threshold || abs(AcY) > threshold || abs(AcZ) > threshold) && !callActive) {
    // Make a call if no call is active and threshold exceeded
    digitalWrite(buzzerPin, HIGH);
    buzzerStartTime = millis();     // Record when buzzer started
    buzzerActive = true; 
    
  }

  // Check if the button is pressed
  if (digitalRead(buttonPin) == LOW && buzzerActive) {
    digitalWrite(buzzerPin, LOW);  // Turn off the buzzer
    buzzerActive = false;          // Reset buzzer active state
  }

  // Turn off buzzer automatically after 20 seconds if the button is not pressed
  if (buzzerActive && millis() - buzzerStartTime > 20000) {
    digitalWrite(buzzerPin, LOW);  // Turn off the buzzer
    buzzerActive = false;  
    makeCall();        // Reset buzzer active state
  }

  while (gpsSerial.available()) {
    char c = gpsSerial.read();
    Serial.write(c);
    gps.encode(c);  // Encode GPS data
  }

  // Check if GPS data is available and valid before sending SMS
  if (gps.location.isUpdated()) {
    Serial.print("Latitude: "); Serial.println(gps.location.lat(), 6);
    Serial.print("Longitude: "); Serial.println(gps.location.lng(), 6);
  }

  delay(1000);  // Wait for a second before next reading
}
