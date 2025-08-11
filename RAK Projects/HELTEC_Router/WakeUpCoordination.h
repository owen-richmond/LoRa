/**
 * @file WakeUpCoordination.h
 * @brief A unified, object-oriented class to manage LoRa node roles and cycles.
 * @version 4.0
 * @details This file provides a single `WakeUpCoordinator` class that can be
 * configured to act as a low-power, timed SENDER, a continuous RECEIVER, or a RELAY.
 */

#ifndef WAKE_UP_COORDINATION_H
#define WAKE_UP_COORDINATION_H

#include <Arduino.h>
#include <heltec_unofficial.h>

//================================================================//
//                  INTEGRATED TIMER PACKET CLASS                 //
//================================================================//

/**
 * @class TimerPacket
 * @brief A simple, serializable data structure to communicate timing and state.
 */
class TimerPacket {
public:
    uint32_t packetId;
    uint16_t messageInterval_s;
    uint16_t wakeWindow_s;
    uint8_t  checksum;

    static const size_t PACKET_SIZE = sizeof(packetId) + sizeof(messageInterval_s) + sizeof(wakeWindow_s) + sizeof(checksum);

    TimerPacket() : packetId(0), messageInterval_s(0), wakeWindow_s(0), checksum(0) {}

    TimerPacket(uint32_t id, uint16_t interval, uint16_t window)
        : packetId(id), messageInterval_s(interval), wakeWindow_s(window), checksum(0) {
        this->checksum = calculateChecksum();
    }

    void serialize(uint8_t* buffer) const {
        size_t offset = 0;
        memcpy(buffer + offset, &packetId, sizeof(packetId));
        offset += sizeof(packetId);
        memcpy(buffer + offset, &messageInterval_s, sizeof(messageInterval_s));
        offset += sizeof(messageInterval_s);
        memcpy(buffer + offset, &wakeWindow_s, sizeof(wakeWindow_s));
        offset += sizeof(wakeWindow_s);
        memcpy(buffer + offset, &checksum, sizeof(checksum));
    }

    bool deserialize(const uint8_t* buffer) {
        size_t offset = 0;
        memcpy(&packetId, buffer + offset, sizeof(packetId));
        offset += sizeof(packetId);
        memcpy(&messageInterval_s, buffer + offset, sizeof(messageInterval_s));
        offset += sizeof(messageInterval_s);
        memcpy(&wakeWindow_s, buffer + offset, sizeof(wakeWindow_s));
        offset += sizeof(wakeWindow_s);
        uint8_t receivedChecksum;
        memcpy(&receivedChecksum, buffer + offset, sizeof(receivedChecksum));
        if (receivedChecksum == calculateChecksum()) {
            this->checksum = receivedChecksum;
            return true;
        }
        return false;
    }

    // A method to deserialize even with a bad checksum, and then fix it.
    void deserializeAndFix(const uint8_t* buffer) {
        size_t offset = 0;
        memcpy(&packetId, buffer + offset, sizeof(packetId));
        offset += sizeof(packetId);
        memcpy(&messageInterval_s, buffer + offset, sizeof(messageInterval_s));
        offset += sizeof(messageInterval_s);
        memcpy(&wakeWindow_s, buffer + offset, sizeof(wakeWindow_s));
        // Now, calculate the correct checksum and store it.
        this->checksum = calculateChecksum();
    }

private:
    uint8_t calculateChecksum() const {
        uint8_t chk = 0;
        const uint8_t* byte_ptr = reinterpret_cast<const uint8_t*>(this);
        for (size_t i = 0; i < (PACKET_SIZE - sizeof(checksum)); ++i) {
            chk ^= byte_ptr[i];
        }
        return chk;
    }
};

#endif // WAKE_UP_COORDINATION_H