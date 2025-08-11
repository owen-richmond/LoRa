/**
 * @file LoRa_Relay_Main.ino
 * @author Gemini
 * @brief Main sketch for a LoRa P2P Relay/Router on a Heltec WiFi LoRa V3.1.
 * @version 1.2
 * @date 2025-07-23
 * @details
 * This node acts as a relay. It listens for packets from a Sender, which are
 * expected to have an INVALID checksum. Upon receipt, this Relay corrects the
 * checksum and re-transmits the now-VALID packet to a final Receiver.
 * It uses the OLED to display its status. This version uses a polling mechanism
 * for simplicity and robustness, avoiding ISR complexity.
 */

#include <Arduino.h>
#include <heltec_unofficial.h>
#include "WakeUpCoordination.h" // Contains the TimerPacket definition

//================================================================//
//                     OLED DISPLAY HELPER                        //
//================================================================//

void displayMessage(const String& line1, const String& line2 = "", const String& line3 = "") {
    display.clear();
    display.drawString(0, 0, line1);
    display.drawString(0, 12, line2);
    display.drawString(0, 24, line3);
    display.display();
    Serial.println(line1 + " " + line2 + " " + line3);
}

//================================================================//
//                COORDINATOR CLASS FOR ROUTER LOGIC              //
//================================================================//

class Coordinator {
public:
    bool isListening;

    Coordinator() : isListening(false) {}

    void begin() {
        displayMessage("Router Starting...");

        // Initialize the radio
        RADIOLIB_OR_HALT(radio.begin());
        RADIOLIB_OR_HALT(radio.setFrequency(915.0));
        RADIOLIB_OR_HALT(radio.setBandwidth(125.0));
        RADIOLIB_OR_HALT(radio.setSpreadingFactor(7));
        RADIOLIB_OR_HALT(radio.setCodingRate(5));
        RADIOLIB_OR_HALT(radio.setOutputPower(22));

        startReceiving();
    }

    void poll() {
        // If we are not in a listening state, do nothing.
        // The logic will transition back to listening after a send or error.
        if (!isListening) {
            return;
        }

        // Check if a packet has been received
        uint16_t state = radio.available();

        if (state == RADIOLIB_ERR_NONE) {
            // A packet was received, stop listening to process it.
            isListening = false;

            uint8_t buffer[TimerPacket::PACKET_SIZE];
            int16_t err = radio.readData(buffer, TimerPacket::PACKET_SIZE);

            displayMessage("Packet Received!", "RSSI: " + String(radio.getRSSI()) + " SNR: " + String(radio.getSNR()));
            delay(1000);

            if (err != RADIOLIB_ERR_NONE) {
                displayMessage("ERROR: readData failed", "Code: " + String(err));
                delay(2000);
                startReceiving(); // Go back to listening
                return;
            }

            // --- Packet Validation and Correction ---
            TimerPacket received_packet;
            if (received_packet.deserialize(buffer)) {
                displayMessage("WARN: Packet is valid", "No correction needed.", "ID: " + String(received_packet.packetId));
                delay(2000);
                startReceiving(); // Don't forward valid packets
                return;
            }

            displayMessage("Checksum Invalid!", "Correcting packet...");
            TimerPacket corrected_packet;
            corrected_packet.deserializeAndFix(buffer);
            delay(1000);

            // --- Send the Corrected Packet ---
            sendPacket(corrected_packet);

        } else if (state != RADIOLIB_ERR_RX_TIMEOUT) {
            // Some other error occurred
            isListening = false;
            displayMessage("RX Error!", "Code: " + String(state));
            delay(2000);
            startReceiving();
        }
    }

private:
    void startReceiving() {
        displayMessage("Listening for packet...");
        radio.startReceive();
        isListening = true;
    }

    void sendPacket(const TimerPacket& packet) {
        displayMessage("Sending corrected...", "ID: " + String(packet.packetId));

        uint8_t buffer[TimerPacket::PACKET_SIZE];
        packet.serialize(buffer);

        // Transmit the packet (this is a blocking call)
        int16_t state = radio.transmit(buffer, TimerPacket::PACKET_SIZE);

        if (state == RADIOLIB_ERR_NONE) {
            displayMessage("Corrected packet sent!");
        } else {
            displayMessage("TX Error!", "Code: " + String(state));
        }

        delay(2000);
        // After sending (or failing), go back to listening.
        startReceiving();
    }
};

//================================================================//
//                       GLOBAL OBJECTS                           //
//================================================================//

Coordinator coordinator;

//================================================================//
//                       SETUP & LOOP                             //
//================================================================//

void setup() {
    Serial.begin(115200);
    heltec_setup();
    display.setFont(ArialMT_Plain_10);

    Serial.println("=========================================");
    Serial.println("        LoRa P2P Relay (Router)          ");
    Serial.println("=========================================");

    coordinator.begin();
}

void loop() {
    // The coordinator's poll() method handles all the logic.
    coordinator.poll();

    // The heltec_loop() handles background tasks for the display.
    heltec_loop();
}
