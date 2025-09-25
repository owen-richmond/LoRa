/**
 * @file main.ino
 * @brief This file contains the implementation of the Heltec LoRa device functions, handling both the host and client roles, with deep sleep functionality.
 */

#include <heltec_unofficial.h> // Includes Heltec library for display and LoRa functionalities
#include <RadioLib.h>          // Includes RadioLib library for LoRa communication
#include "Timer.h"             // Includes the Timer class header
#include "WakeUpCoordination.h" // Includes WakeUpCoordination header
#include "esp_sleep.h"         // Includes ESP sleep functions

// Radio configuration
#define FREQUENCY 915.0        // Frequency for LoRa communication
#define BANDWIDTH 250.0        // Bandwidth for LoRa communication
#define SPREADING_FACTOR 9     // Spreading factor for LoRa communication
#define TRANSMIT_POWER 20      // Maximum transmit power for LoRa

#define IS_HOST false          // Define the role of the device (true for host, false for client)
#define LED_BRIGHTNESS 20      // Set LED brightness to 20%

TaskHandle_t taskHandle;       // Task handle for the secondary core task
RTC_DATA_ATTR bool firstRunAfterDeepSleep = true; // Flag to detect first run after deep sleep
RTC_DATA_ATTR Timer timer(0, 0, 0, 0);            // Declare Timer object
RTC_DATA_ATTR uint32_t deepSleepWakeupCount = 0;  // Counter for deep sleep wakeups

WakeUpCoordination coordinator; // Declare an instance of WakeUpCoordination

/**
 * @brief Initialize Heltec display and LoRa
 */
void initializeHeltec() {
  heltec_setup(); // Initialize Heltec display
  Serial.begin(115200); // Initialize serial communication at 115200 baud rate

  // Initialize the LoRa radio with specified parameters
  int state = radio.begin(FREQUENCY, BANDWIDTH, SPREADING_FACTOR, 7, RADIOLIB_SX126X_SYNC_WORD_PRIVATE, TRANSMIT_POWER, 8, 1.6, false);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("Radio initialization successful!");
  } else {
    Serial.println("Radio initialization failed!");
    while (1); // Halt if radio initialization fails
  }
  radio.setDio1Action(NULL); // Set DIO1 action to NULL

  // Display battery percentage and wakeup count
  display.clear();
  float batteryPercentage = heltec_battery_percent();
  String batteryString = "Battery: " + String(batteryPercentage, 1) + "%";
  String wakeupString = "Wakeups: " + String(deepSleepWakeupCount);
  display.drawString(0, 0, batteryString);
  display.drawString(0, 10, wakeupString);
  display.display();
  delay(300); // Display for 0.3 seconds

  // Clear the display after showing initial information
  display.clear();
  display.display();
}

/**
 * @brief Setup function to initialize the device
 */
void setup() {
  Serial.begin(115200); // Initialize serial communication at 115200 baud rate
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause(); // Detect the wake-up reason

  // Handle wake-up reasons
  if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER) {
    Serial.println("Waking up from deep sleep.");
    deepSleepWakeupCount++; // Increment wakeup count on deep sleep wakeup
  } else {
    Serial.println("Normal boot.");
    firstRunAfterDeepSleep = true; // Set flag for first run after power on
    deepSleepWakeupCount = 0; // Reset wakeup count on normal boot
  }

  initializeHeltec(); // Initialize Heltec display and LoRa

  // Create task on core 1
  xTaskCreatePinnedToCore(
    loopTaskCore1, // Function to implement the task
    "LoopTaskCore1", // Name of the task
    10000, // Stack size in words
    NULL, // Task input parameter
    1, // Priority of the task
    &taskHandle, // Task handle
    0); // Core where the task should run (0 for PRO_CPU, 1 for APP_CPU)
}

/**
 * @brief Reset the state variables
 */
void resetState() {
  timer = Timer(0, 0, 0, 0); // Reset Timer object
}

/**
 * @brief Main loop function (empty as tasks are handled by the secondary core task)
 */
void loop() {
  // Main loop is empty, all work is done in loopTaskCore1
}

/**
 * @brief Function to display messages on the OLED
 * @param line1 First line of text to display
 * @param line2 Second line of text to display
 */
void displayFunction(const String& line1, const String& line2) {
  display.clear();
  display.drawString(0, 0, line1);
  display.drawString(0, 10, line2);
  display.display();
}

/**
 * @brief Task function to run on the second core
 * @param parameter Pointer to the task parameter (not used)
 */
void loopTaskCore1(void *parameter) {
  while (true) {
    heltec_loop(); // Loop function for Heltec tasks

    if (firstRunAfterDeepSleep) {
      resetState(); // Reset the state after waking up from deep sleep
      firstRunAfterDeepSleep = false;
      Serial.println("Reinitialized after deep sleep or power on.");
    }

    uint32_t sleepDuration = coordinator.coordinate(timer, IS_HOST, radio, heltec_led, displayFunction);

    Serial.print("Going to sleep for ");
    Serial.print(sleepDuration);
    Serial.println(" seconds.");

    heltec_deep_sleep(sleepDuration); // Enter deep sleep for calculated duration

    vTaskDelay(1); // Yield to other tasks
  }
}
