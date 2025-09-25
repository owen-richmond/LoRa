/**
 * @file LoRa_Client_Main.ino
 * @author Gemini
 * @brief Main sketch for a LoRa P2P Client (Receiver) using the unified WakeUpCoordinator.
 * @version 5.1
 * @date 2025-07-22
 * @details
 * This main file demonstrates the clean, object-oriented approach you envisioned.
 * Its sole responsibility is to configure this node as a RECEIVER and then
 * hand off all control to the WakeUpCoordinator object.
 *
 * NOTE: The corresponding Sender for this project is intentionally sending
 * an invalid packet. This Receiver should correctly identify the checksum
 * error and discard the packet. This setup is for a future project involving
 * a relay node that will correct the packet.
 */

#include <Arduino.h>
#include "WakeUpCoordination.h" // The unified, role-based coordinator class

//================================================================//
//                  NODE CONFIGURATION
//================================================================//
// 1. Set the role for this device.
//    For the client, this must be RECEIVER.
const WakeUpCoordinator::Role NODE_ROLE = WakeUpCoordinator::RECEIVER;

// 2. Define timing parameters.
//    NOTE: These are ignored for the RECEIVER role but are shown here for clarity.
//    They would be used if you changed the NODE_ROLE to SENDER.
const uint32_t SEND_INTERVAL_MS = 10000; // 10 seconds
const uint32_t WAKE_WINDOW_MS   = 3000;  // 3 seconds


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
    Serial.println("      LoRa P2P Client (Receiver)         ");
    Serial.println("=========================================");

    // Initialize the coordinator. It will automatically configure the radio
    // for the RECEIVER role based on the NODE_ROLE setting.
    coordinator.begin(SEND_INTERVAL_MS, WAKE_WINDOW_MS);
}


//================================================================//
//                        MAIN LOOP
//================================================================//
void loop() {
    // The coordinator's run() method handles all logic. For a RECEIVER,
    // this function does nothing, as all work is done in the OnRxDone callback.
    // This leaves the main loop free for other application tasks.
    coordinator.run();

    // We can add a simple heartbeat LED to confirm the device is running.
    digitalWrite(LED_BUILTIN, HIGH);
    delay(50);
    digitalWrite(LED_BUILTIN, LOW);
    delay(1950);
}
