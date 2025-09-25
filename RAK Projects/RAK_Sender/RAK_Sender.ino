/**
 * @file LoRa_Host_Main.ino
 * @author Gemini
 * @brief Main sketch for a LoRa P2P Host (Sender) using the unified WakeUpCoordinator.
 * @version 5.1
 * @date 2025-07-22
 * @details
 * This main file demonstrates the clean, object-oriented approach.
 * Its sole responsibility is to configure this node as a SENDER and then
 * hand off all control to the WakeUpCoordinator object, which handles the
 * timed wake-up, CAD, and send logic.
 *
 * NOTE: This version is intentionally modified to send a packet with an
 * invalid checksum. This is to simulate a scenario where a relay node
 * would be required to validate and forward a corrected packet. The receiver
 * should identify this packet as invalid.
 */

#include <Arduino.h>
#include "WakeUpCoordination.h" // The unified, role-based coordinator class

//================================================================//
//               a   NODE CONFIGURATION
//================================================================//
// 1. Set the role for this device.
//    For the host, this must be SENDER.
const WakeUpCoordinator::Role NODE_ROLE = WakeUpCoordinator::SENDER;

// 2. Define timing parameters for the SENDER role.
const uint32_t SEND_INTERVAL_MS = 5000; // The total cycle time (10 seconds).
const uint32_t WAKE_WINDOW_MS   = 500;  // The active window to try and send (3 seconds).


//================================================================//
//              GLOBAL OBJECT INSTANTIATION
//================================================================//
// 1. Create the coordinator object with its designated role.
WakeUpCoordinator coordinator(NODE_ROLE);

// 2. Create the global pointer that the callback functions in
//    WakeUpCoordination.h need to access our coordinator object.
WakeUpCoordinator* g_Coordinator = &coordinator;


//================================================================//
//                       SETUP FUNCTION
//================================================================//
void setup() {
    // Initialize the built-in LED for heartbeat feedback
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    // Initialize Serial for debugging output
    Serial.begin(115200);
    time_t timeout = millis();
    while (!Serial) {
        if ((millis() - timeout) < 5000) {
            delay(100); // Wait for serial port to connect
        } else {
            break;
        }
    }

    Serial.println("=========================================");
    Serial.println("        LoRa P2P Host (Sender)           ");
    Serial.println("=========================================");

    // Initialize the coordinator. It will automatically configure the radio
    // for the SENDER role and use the timing parameters defined above.
    coordinator.begin(SEND_INTERVAL_MS, WAKE_WINDOW_MS);
}


//================================================================//
//                        MAIN LOOP
//================================================================//
void loop() {
    // The coordinator's run() method handles all logic for the SENDER.
    // This includes managing the 10-second cycle, waking up, performing CAD,
    // sending the packet, and putting the radio to sleep.
    coordinator.run();

    // The main loop is now free for other application tasks.
    // We add a small delay to be friendly to the CPU.
    delay(10);
}
