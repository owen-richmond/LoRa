#ifndef TIMER_H // Prevents multiple inclusions of this header file
#define TIMER_H

#include <Arduino.h> // Includes the Arduino core library

// Timer class definition
class Timer {
public:
    // Constructor that initializes the Timer object with provided values
    Timer(time_t currentTime, uint16_t messageInterval, uint16_t waitTime, uint8_t sleepState)
        : currentTime(currentTime), messageInterval(messageInterval), waitTime(waitTime), sleepState(sleepState & 0x03) {}

    // Constructor that initializes the Timer object from a serialized data array
    Timer(const uint8_t* data) {
        if (!deserialize(data)) {
            // Handle deserialization error (e.g., log an error, set default values, etc.)
            currentTime = 0;
            messageInterval = 0;
            waitTime = 0;
            sleepState = 0;
        }
    }

    // Getter for the current time
    time_t getCurrentTime() const { return currentTime; }
    // Getter for the message interval
    uint16_t getMessageInterval() const { return messageInterval; }
    // Getter for the wait time
    uint16_t getWaitTime() const { return waitTime; }
    // Getter for the sleep state
    uint8_t getSleepState() const { return sleepState; }

    // Returns the current time as a formatted string
    String getTimeString(time_t timeVal) const {
        char buffer[26];
        struct tm* tm_info = localtime(&timeVal);
        strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
        return String(buffer);
    }

    // Serializes the Timer object into a data array with checksum
    void serialize(uint8_t* data) const {
        memcpy(data, &currentTime, sizeof(currentTime)); // Copies currentTime to the data array
        memcpy(data + sizeof(currentTime), &messageInterval, sizeof(messageInterval)); // Copies messageInterval to the data array
        memcpy(data + sizeof(currentTime) + sizeof(messageInterval), &waitTime, sizeof(waitTime)); // Copies waitTime to the data array
        memcpy(data + sizeof(currentTime) + sizeof(messageInterval) + sizeof(waitTime), &sleepState, sizeof(sleepState)); // Copies sleepState to the data array

        uint8_t checksum = calculateChecksum(data, sizeof(currentTime) + sizeof(messageInterval) + sizeof(waitTime) + sizeof(sleepState));
        memcpy(data + sizeof(currentTime) + sizeof(messageInterval) + sizeof(waitTime) + sizeof(sleepState), &checksum, sizeof(checksum)); // Adds checksum to the data array
    }

    // Deserializes the Timer object from a data array and verifies checksum
    bool deserialize(const uint8_t* data) {
        uint8_t checksum = data[sizeof(currentTime) + sizeof(messageInterval) + sizeof(waitTime) + sizeof(sleepState)];
        uint8_t calculatedChecksum = calculateChecksum(data, sizeof(currentTime) + sizeof(messageInterval) + sizeof(waitTime) + sizeof(sleepState));
        if (checksum != calculatedChecksum) {
            return false; // Checksum mismatch, data is corrupted
        }

        memcpy(&currentTime, data, sizeof(currentTime)); // Copies currentTime from the data array
        memcpy(&messageInterval, data + sizeof(currentTime), sizeof(messageInterval)); // Copies messageInterval from the data array
        memcpy(&waitTime, data + sizeof(currentTime) + sizeof(messageInterval), sizeof(waitTime)); // Copies waitTime from the data array
        memcpy(&sleepState, data + sizeof(currentTime) + sizeof(messageInterval) + sizeof(waitTime), sizeof(sleepState)); // Copies sleepState from the data array
        return true; // Data is valid
    }

    // Calculates checksum for the given data array
    uint8_t calculateChecksum(const uint8_t* data, size_t length) const {
        uint8_t checksum = 0;
        for (size_t i = 0; i < length; ++i) {
            checksum ^= data[i];
        }
        return checksum;
    }

private:
    time_t currentTime; // Stores the current time
    uint16_t messageInterval; // Stores the message interval
    uint16_t waitTime; // Stores the wait time
    uint8_t sleepState; // Stores the sleep state (2-bit value)
};

#endif // TIMER_H