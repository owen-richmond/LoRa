#include <heltec_unofficial.h>
#include <RadioLib.h>
#include "Timer.h"
#include "WakeUpCoordination.h"
#include "esp_sleep.h"

// Radio configuration
#define FREQUENCY 915.0
#define BANDWIDTH 250.0
#define SPREADING_FACTOR 9
#define TRANSMIT_POWER 0

#define IS_HOST true
#define LED_BRIGHTNESS 20 // Set LED brightness to 20%
#define MESSAGE_INTERVAL 30 // Set message interval to 5 seconds

TaskHandle_t taskHandle;
RTC_DATA_ATTR bool firstRunAfterDeepSleep = true;
RTC_DATA_ATTR Timer timer(0, 0, 0, 0);
RTC_DATA_ATTR uint32_t deepSleepWakeupCount = 0; // Counter for deep sleep wakeups

WakeUpCoordination coordinator;

void initializeHeltec() {
  heltec_setup();
  Serial.begin(115200);

  // Initialize radio
  int state = radio.begin(FREQUENCY, BANDWIDTH, SPREADING_FACTOR, 7, RADIOLIB_SX126X_SYNC_WORD_PRIVATE, TRANSMIT_POWER, 8, 1.6, false);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("Radio initialization successful!");
  } else {
    Serial.println("Radio initialization failed!");
    while (1);
  }
  radio.setDio1Action(NULL);

  // Display battery percentage and wakeup count
  display.clear();
  float batteryPercentage = heltec_battery_percent();
  String batteryString = "Battery: " + String(batteryPercentage, 1) + "%";
  String wakeupString = "Wakeups: " + String(deepSleepWakeupCount);
  display.drawString(0, 0, batteryString);
  display.drawString(0, 10, wakeupString);
  display.display();
  delay(300); // Display for 1 second

  display.clear();
  display.display();
}

void setup() {
  Serial.begin(115200);
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

  if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER) {
    Serial.println("Waking up from deep sleep.");
    deepSleepWakeupCount++;
  } else {
    Serial.println("Normal boot.");
    firstRunAfterDeepSleep = true;
    deepSleepWakeupCount = 0;
  }

  initializeHeltec();

  // Create task on core 1
  xTaskCreatePinnedToCore(
    loopTaskCore1,
    "LoopTaskCore1",
    10000,
    NULL,
    1,
    &taskHandle,
    0);
}

void resetState() {
  timer = Timer(0, 0, 0, 0);
}

void loop() {
  // Main loop is empty, all work is done in loopTaskCore1
}

void displayFunction(const String& line1, const String& line2) {
  display.clear();
  display.drawString(0, 0, line1);
  display.drawString(0, 10, line2);
  display.display();
}

void loopTaskCore1(void *parameter) {
  while (true) {
    heltec_loop();

    if (firstRunAfterDeepSleep) {
      resetState();
      firstRunAfterDeepSleep = false;
      Serial.println("Reinitialized after deep sleep or power on.");
    }

    uint32_t sleepDuration = coordinator.coordinate(timer, IS_HOST, radio, heltec_led, displayFunction, MESSAGE_INTERVAL);

    Serial.print("Going to sleep for ");
    Serial.print(sleepDuration);
    Serial.println(" seconds.");

    heltec_deep_sleep(sleepDuration);

    vTaskDelay(1);
  }
}