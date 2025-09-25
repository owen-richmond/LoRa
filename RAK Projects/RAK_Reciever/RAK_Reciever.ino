/**
 * @file LoRa_Client_Main.ino
 * @author Gemini
 * @brief Main sketch for a LoRa P2P Client (Receiver).
 * @version 6.1 - ID-Based Routing (Code Order Fix)
 * @date 2025-07-23
 * @details
 * This node listens for packets specifically forwarded by the ROUTER.
 * It will ignore any packets that are not addressed to it or that do not
 * originate from the designated ROUTER_ID.
 */

#include <Arduino.h>
#include "WakeUpCoordination.h" // Contains network configs and packet definition

//================================================================//
//                WAKEUP COORDINATOR CLASS
//================================================================//
// The class must be defined before it is used.

class WakeUpCoordinator {
public:
    enum Role { SENDER, RECEIVER };

private:
    Role _role;
    RadioEvents_t _radioEvents;

public:
    WakeUpCoordinator(Role role) : _role(role) {}

    void begin() {
        lora_rak4630_init();

        if (_role == RECEIVER) {
            // Forward declare the callbacks so the class knows about them.
            void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr);
            void OnRxTimeout(void);
            void OnRxError(void);

            _radioEvents.RxDone = OnRxDone;
            _radioEvents.RxTimeout = OnRxTimeout;
            _radioEvents.RxError = OnRxError;
            Radio.Init(&_radioEvents);
            Radio.SetChannel(915000000);
            Radio.SetRxConfig(MODEM_LORA, 0, 7, 1, 0, 8, 0, false, 0, true, 0, 0, false, true);
            Radio.Rx(0); // Start listening
        }
    }

    void run() {
        // Receiver is entirely event-driven
    }
};

//================================================================//
//                  NODE CONFIGURATION
//================================================================//
const WakeUpCoordinator::Role NODE_ROLE = WakeUpCoordinator::RECEIVER;
const NodeID MY_ID = RECEIVER_ID;

//================================================================//
//              GLOBAL OBJECTS & POINTERS
//================================================================//
WakeUpCoordinator coordinator(NODE_ROLE);
WakeUpCoordinator* g_Coordinator = &coordinator; // g_Coordinator is not used in receiver, but kept for consistency

//================================================================//
//              CALLBACKS FOR RADIO EVENTS
//================================================================//
void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
    if (size != TimerPacket::PACKET_SIZE) {
        Serial.println("Received packet with invalid size. Ignoring.");
        Radio.Rx(0);
        return;
    }

    TimerPacket received_packet;
    if (!received_packet.deserialize(payload)) {
        Serial.println("Received packet with invalid checksum. Ignoring.");
        Radio.Rx(0);
        return;
    }

    // --- ID-Based Routing Logic ---
    if (received_packet.destinationId != MY_ID) {
        Serial.printf("Received packet not for me (for %d). Ignoring.\n", received_packet.destinationId);
        Radio.Rx(0);
        return;
    }

    if (received_packet.sourceId != ROUTER_ID) {
        Serial.printf("Received packet from wrong source (%d). Ignoring.\n", received_packet.sourceId);
        Radio.Rx(0);
        return;
    }

    // --- If we get here, the packet is valid and for us ---
    Serial.println("\n--- VALID PACKET RECEIVED ---");
    Serial.printf("  Source: %d, Dest: %d, ID: %lu\n", received_packet.sourceId, received_packet.destinationId, received_packet.packetId);
    Serial.printf("  RSSI: %d dBm, SNR: %d\n", rssi, snr);

    Radio.Rx(0); // Go back to listening
}

void OnRxTimeout(void) {
    Serial.println("RX Timeout");
    Radio.Rx(0);
}

void OnRxError(void) {
    Serial.println("RX Error");
    Radio.Rx(0);
}

//================================================================//
//                       SETUP & LOOP
//================================================================//
void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
    Serial.begin(115200);
    time_t timeout = millis();
    while (!Serial) {
        if ((millis() - timeout) < 5000) delay(100);
        else break;
    }

    Serial.println("=========================================");
    Serial.println("      LoRa P2P Client (Receiver)         ");
    Serial.println("=========================================");

    coordinator.begin();
}

void loop() {
    // The receiver is event-driven, but we can add a heartbeat.
    digitalWrite(LED_BUILTIN, HIGH);
    delay(50);
    digitalWrite(LED_BUILTIN, LOW);
    delay(1950);
}
