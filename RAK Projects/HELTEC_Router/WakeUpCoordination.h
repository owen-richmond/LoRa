/**
 * @file WakeUpCoordination.h
 * @brief Header for Heltec LoRa router (RadioLib compatible)
 * @version 6.0 - Heltec Compatible
 * @date 2025-08-11
 * @details
 * This header works with RadioLib and heltec_unofficial library
 */

#ifndef WAKE_UP_COORDINATION_H
#define WAKE_UP_COORDINATION_H

#include <Arduino.h>

//================================================================//
//                  NETWORK CONFIGURATION                         //
//================================================================//

enum NodeID : uint8_t {
    SENDER_ID   = 10,
    ROUTER_ID   = 20,
    RECEIVER_ID = 30
};

//================================================================//
//                  INTEGRATED TIMER PACKET CLASS                 //
//================================================================//

class TimerPacket {
public:
    // --- Packet Data Fields ---
    uint8_t  sourceId;
    uint8_t  destinationId;
    uint32_t packetId;
    uint16_t messageInterval_s;
    uint16_t wakeWindow_s;
    uint8_t  checksum;

    // The size is calculated manually to be explicit and consistent
    static const size_t PACKET_SIZE = 1 + 1 + 4 + 2 + 2 + 1; // 11 bytes

    TimerPacket() : sourceId(0), destinationId(0), packetId(0), messageInterval_s(0), wakeWindow_s(0), checksum(0) {}

    // Constructor for creating a new packet
    TimerPacket(uint8_t src, uint8_t dest, uint32_t id, uint16_t interval, uint16_t window)
        : sourceId(src), destinationId(dest), packetId(id), messageInterval_s(interval), wakeWindow_s(window) {
        recomputeChecksum();
    }

    // --- Safe Serialization ---
    // Copies each field byte-by-byte into the buffer, avoiding struct padding.
    void serialize(uint8_t* buffer) const {
        size_t offset = 0;
        memcpy(buffer + offset, &sourceId, sizeof(sourceId));
        offset += sizeof(sourceId);
        memcpy(buffer + offset, &destinationId, sizeof(destinationId));
        offset += sizeof(destinationId);
        memcpy(buffer + offset, &packetId, sizeof(packetId));
        offset += sizeof(packetId);
        memcpy(buffer + offset, &messageInterval_s, sizeof(messageInterval_s));
        offset += sizeof(messageInterval_s);
        memcpy(buffer + offset, &wakeWindow_s, sizeof(wakeWindow_s));
        offset += sizeof(wakeWindow_s);
        memcpy(buffer + offset, &checksum, sizeof(checksum));
    }

    // --- Safe Deserialization ---
    // Copies each field byte-by-byte from the buffer, avoiding struct padding.
    bool deserialize(const uint8_t* buffer) {
        size_t offset = 0;
        memcpy(&sourceId, buffer + offset, sizeof(sourceId));
        offset += sizeof(sourceId);
        memcpy(&destinationId, buffer + offset, sizeof(destinationId));
        offset += sizeof(destinationId);
        memcpy(&packetId, buffer + offset, sizeof(packetId));
        offset += sizeof(packetId);
        memcpy(&messageInterval_s, buffer + offset, sizeof(messageInterval_s));
        offset += sizeof(messageInterval_s);
        memcpy(&wakeWindow_s, buffer + offset, sizeof(wakeWindow_s));
        offset += sizeof(wakeWindow_s);
        
        // Save the received checksum
        uint8_t receivedChecksum;
        memcpy(&receivedChecksum, buffer + offset, sizeof(receivedChecksum));

        // Verify checksum based on the data we just deserialized
        if (receivedChecksum == calculateChecksum()) {
            this->checksum = receivedChecksum;
            return true;
        }
        return false;
    }

    // Method to recalculate and set the checksum.
    void recomputeChecksum() {
        this->checksum = calculateChecksum();
    }

private:
    // --- Safe Checksum Calculation ---
    // Calculates checksum from the class fields directly, not raw memory,
    // which makes it immune to padding and memory layout.
    uint8_t calculateChecksum() const {
        uint8_t chk = 0;

        // XOR single-byte fields
        chk ^= sourceId;
        chk ^= destinationId;
        
        // XOR each byte of the multi-byte fields
        const uint8_t* p_packetId = reinterpret_cast<const uint8_t*>(&packetId);
        for (size_t i = 0; i < sizeof(packetId); ++i) {
            chk ^= p_packetId[i];
        }

        const uint8_t* p_interval = reinterpret_cast<const uint8_t*>(&messageInterval_s);
        for (size_t i = 0; i < sizeof(messageInterval_s); ++i) {
            chk ^= p_interval[i];
        }

        const uint8_t* p_window = reinterpret_cast<const uint8_t*>(&wakeWindow_s);
        for (size_t i = 0; i < sizeof(wakeWindow_s); ++i) {
            chk ^= p_window[i];
        }

        return chk;
    }
};

#endif // WAKE_UP_COORDINATION_H
