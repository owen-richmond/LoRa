#define HELTEC_POWER_BUTTON
#include <heltec_unofficial.h>
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>

// Define GPS pins
#define GPS_RX_PIN 45
#define GPS_TX_PIN 46

// Create a TinyGPSPlus object
TinyGPSPlus gps;

// Use Serial2 for the GPS
HardwareSerial gpsSerial(2);

unsigned long startMillis;

void setup() {
  // Initialize the serial communication with the computer
  Serial.begin(115200);

  // Initialize the serial communication with the GPS module
  gpsSerial.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);

  // Initialize the Heltec display
  heltec_setup();
  display.setFont(ArialMT_Plain_10); // Set font size for the display
  display.clear(); // Clear the display

  startMillis = millis();
}

void loop() {
  // Check if data is available from the GPS module
  while (gpsSerial.available() > 0) {
    if (gps.encode(gpsSerial.read())) {
      displayInfo();
    }
  }

  if (millis() - startMillis > 5000 && gps.charsProcessed() < 10) {
    display.clear();
    display.drawString(0, 0, "No GPS detected: check wiring.");
    display.display();
    Serial.println(F("No GPS detected: check wiring."));
    while (true);
  }

  delay(1000); // Update every second
}

void displayInfo() {
  display.clear();
  Serial.print(F("Location: "));

  if (gps.location.isValid()) {
    String lat = String(gps.location.lat(), 6);
    String lng = String(gps.location.lng(), 6);

    display.drawString(0, 0, "Lat: " + lat);
    display.drawString(0, 10, "Lng: " + lng);
    display.drawString(0, 20, "Fix: Valid");

    Serial.print(lat);
    Serial.print(F(", "));
    Serial.println(lng);
  } else if (millis() - startMillis < 120000) { // Within the first 2 minutes
    display.drawString(0, 0, "Searching for satellites...");
    display.drawString(0, 10, "Please wait...");

    Serial.println(F("Searching for satellites... Please wait..."));
  } else {
    display.drawString(0, 0, "Lat: INVALID");
    display.drawString(0, 10, "Lng: INVALID");
    display.drawString(0, 20, "Fix: INVALID");

    Serial.println(F("Location: INVALID"));
  }

  // Display altitude if available
  if (gps.altitude.isValid()) {
    String alt = String(gps.altitude.meters());
    display.drawString(0, 30, "Alt: " + alt + " m");
    Serial.print(F("Altitude: "));
    Serial.print(alt);
    Serial.println(F(" m"));
  } else {
    display.drawString(0, 30, "Alt: INVALID");
    Serial.println(F("Altitude: INVALID"));
  }

  // Update the display
  display.display();
}

void updateSerial(){
  delay(500);
  while (Serial.available()) {
    gpsSerial.write(Serial.read()); // Forward what Serial received to Serial2
  }
  while (gpsSerial.available()) {
    Serial.write(gpsSerial.read()); // Forward what Serial2 received to Serial
  }
}
