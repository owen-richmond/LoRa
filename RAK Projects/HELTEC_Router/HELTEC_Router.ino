/**
 * @file LoRa_Router_Main.ino
 * @author Fixed to match RAK sender exactly
 * @brief Heltec router that receives from RAK sender and forwards to RAK receiver
 * @version 5.0 - Parameter Matched
 * @date 2025-08-11
 * @details
 * This router uses the exact same LoRa parameters as your RAK sender/receiver.
 * Key fix: Using same power level, sync word, and all timing parameters.
 */

#include <Arduino.h>
#include <heltec_unofficial.h>
#include "WakeUpCoordination.h"

//================================================================//
//                  NODE CONFIGURATION
//================================================================//
const NodeID MY_ID = ROUTER_ID;

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
    unsigned long lastActivityTime;
    static const unsigned long RX_TIMEOUT_MS = 30000; // 30 second timeout

    Coordinator() : isListening(false), lastActivityTime(0) {}

    void begin() {
        displayMessage("Router Starting...", "ID: " + String(MY_ID));

        // Initialize the radio with EXACT same parameters as RAK sender
        RADIOLIB_OR_HALT(radio.begin());
        
        // Match your RAK sender parameters EXACTLY:
        // Radio.SetChannel(915000000) = 915.0 MHz
        // SetTxConfig: SF=7, BW=125kHz, CR=4/5, Power=22, Preamble=8
        RADIOLIB_OR_HALT(radio.setFrequency(915.0));        // 915 MHz
        RADIOLIB_OR_HALT(radio.setBandwidth(125.0));        // 125 kHz (same as RAK BW=0)
        RADIOLIB_OR_HALT(radio.setSpreadingFactor(7));      // SF7 (same as RAK)
        RADIOLIB_OR_HALT(radio.setCodingRate(5));           // CR 4/5 (same as RAK CR=1)
        RADIOLIB_OR_HALT(radio.setOutputPower(22));         // 22 dBm (same as RAK)
        RADIOLIB_OR_HALT(radio.setPreambleLength(8));       // 8 symbols (same as RAK)
        RADIOLIB_OR_HALT(radio.setCRC(true));               // CRC enabled (same as RAK)
        
        // Set sync word to LoRa default (this is critical!)
        RADIOLIB_OR_HALT(radio.setSyncWord(0x12));          // Default LoRa sync word (not 0x1424)

        startReceiving();
    }

    void poll() {
        if (!isListening) return;

        // Check for timeout and restart if needed
        if (millis() - lastActivityTime > RX_TIMEOUT_MS) {
            displayMessage("RX Timeout", "Restarting...");
            startReceiving();
            return;
        }

        // Check if a packet has been received
        uint16_t state = radio.available();

        if (state == RADIOLIB_ERR_NONE) {
            // A packet was received
            isListening = false;
            lastActivityTime = millis();

            uint8_t buffer[TimerPacket::PACKET_SIZE];
            int16_t err = radio.readData(buffer, TimerPacket::PACKET_SIZE);

            if (err != RADIOLIB_ERR_NONE) {
                displayMessage("ERROR: readData failed", "Code: " + String(err));
                delay(1000);
                startReceiving();
                return;
            }

            // Get signal quality for debugging
            float rssi = radio.getRSSI();
            float snr = radio.getSNR();
            
            displayMessage("Packet Received!", "RSSI: " + String(rssi), "SNR: " + String(snr));
            delay(1000);

            // Process the packet
            TimerPacket received_packet;
            if (!received_packet.deserialize(buffer)) {
                displayMessage("ERROR: Checksum fail", "Bad packet data");
                delay(1000);
                startReceiving();
                return;
            }

            displayMessage("Valid Packet!", "From: " + String(received_packet.sourceId), "To: " + String(received_packet.destinationId));
            delay(1000);

            // --- ROUTING LOGIC ---
            if (received_packet.sourceId == MY_ID) {
                displayMessage("Ignoring own packet.");
                delay(1000);
                startReceiving();
                return;
            }

            if (received_packet.sourceId == SENDER_ID && received_packet.destinationId == RECEIVER_ID) {
                displayMessage("Packet needs routing!", "Re-addressing...", "ID: " + String(received_packet.packetId));
                delay(1000);

                // Forward the packet
                TimerPacket forwarded_packet = received_packet;
                forwarded_packet.sourceId = MY_ID;  // Change source to router
                forwarded_packet.recomputeChecksum();
                sendPacket(forwarded_packet);
            } else {
                displayMessage("Packet not for routing", "Src:" + String(received_packet.sourceId), "Dst:" + String(received_packet.destinationId));
                delay(2000);
                startReceiving();
            }

        } else if (state != RADIOLIB_ERR_RX_TIMEOUT) {
            // Some other error occurred
            isListening = false;
            displayMessage("RX Error!", "Code: " + String(state));
            delay(2000);
            startReceiving();
        }
        // If state == RADIOLIB_ERR_RX_TIMEOUT, continue polling
    }

private:
    void startReceiving() {
        displayMessage("Listening for packets...");
        lastActivityTime = millis();
        
        // Put radio in standby first
        radio.standby();
        delay(10);
        
        // Start receiving
        int16_t state = radio.startReceive();
        if (state == RADIOLIB_ERR_NONE) {
            isListening = true;
        } else {
            isListening = false;
            displayMessage("RX Start Failed!", "Code: " + String(state));
            delay(2000);
            // Try again
            startReceiving();
        }
    }

    void sendPacket(const TimerPacket& packet) {
        displayMessage("Forwarding packet...", "From: " + String(packet.sourceId), "To: " + String(packet.destinationId));

        uint8_t buffer[TimerPacket::PACKET_SIZE];
        packet.serialize(buffer);

        // Put radio in standby before transmit
        radio.standby();
        delay(10);

        // Transmit the packet
        int16_t state = radio.transmit(buffer, TimerPacket::PACKET_SIZE);

        if (state == RADIOLIB_ERR_NONE) {
            displayMessage("Packet forwarded!", "Success!");
        } else {
            displayMessage("TX Error!", "Code: " + String(state));
        }

        delay(2000);
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
    Serial.println("     LoRa P2P Router (Heltec V3.1)     ");
    Serial.println("     Matched to RAK Parameters         ");
    Serial.println("=========================================");

    coordinator.begin();
}

void loop() {
    coordinator.poll();
    heltec_loop();
}
