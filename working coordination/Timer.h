/**
 * @file Timer.h
 * @brief This file contains the definition of the Timer class, which is used to manage timing intervals, wait times, and sleep states for LoRa communication.
 */

#ifndef TIMER_H // Prevents multiple inclusions of this header file
#define TIMER_H

#include <Arduino.h> // Includes the Arduino core library

/**
 * @class Timer
 * @brief Manages timing intervals, wait times, and sleep states for LoRa communication.
 */
class Timer {
public:
    /**
     * @brief Constructor that initializes the Timer object with provided values.
     * @param currentTime The current time.
     * @param messageInterval The interval between messages.
     * @param waitTime The wait time before sending the next message.
     * @param sleepState The sleep state of the device.
     */
    Timer(time_t currentTime, uint16_t messageInterval, uint16_t waitTime, uint8_t sleepState)
        : currentTime(currentTime), messageInterval(messageInterval), waitTime(waitTime), sleepState(sleepState & 0x03) {}

    /**
     * @brief Constructor that initializes the Timer object from a serialized data array.
     * @param data The serialized data array.
     */
    Timer(const uint8_t* data) {
        if (!deserialize(data)) {
            // Handle deserialization error (e.g., log an error, set default values, etc.)
            currentTime = 0;
            messageInterval = 0;
            waitTime = 0;
            sleepState = 0;
        }
    }

    /**
     * @brief Getter for the current time.
     * @return The current time.
     */
    time_t getCurrentTime() const { return currentTime; }

    /**
     * @brief Getter for the message interval.
     * @return The message interval.
     */
    uint16_t getMessageInterval() const { return messageInterval; }

    /**
     * @brief Getter for the wait time.
     * @return The wait time.
     */
    uint16_t getWaitTime() const { return waitTime; }

    /**
     * @brief Getter for the sleep state.
     * @return The sleep state.
     */
    uint8_t getSleepState() const { return sleepState; }

    /**
     * @brief Returns the current time as a formatted string.
     * @param timeVal The time value to format.
     * @return The formatted time string.
     */
    String getTimeString(time_t timeVal) const {
        char buffer[26];
        struct tm* tm_info = localtime(&timeVal);
        strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
        return String(buffer);
    }

    /**
     * @brief Serializes the Timer object into a data array with checksum.
     * @param data The data array to serialize into.
     */
    void serialize(uint8_t* data) const {
        memcpy(data, &currentTime, sizeof(currentTime)); // Copies currentTime to the data array
        memcpy(data + sizeof(currentTime), &messageInterval, sizeof(messageInterval)); // Copies messageInterval to the data array
        memcpy(data + sizeof(currentTime) + sizeof(messageInterval), &waitTime, sizeof(waitTime)); // Copies waitTime to the data array
        memcpy(data + sizeof(currentTime) + sizeof(messageInterval) + sizeof(waitTime), &sleepState, sizeof(sleepState)); // Copies sleepState to the data array

        uint8_t checksum = calculateChecksum(data, sizeof(currentTime) + sizeof(messageInterval) + sizeof(waitTime) + sizeof(sleepState));
        memcpy(data + sizeof(currentTime) + sizeof(messageInterval) + sizeof(waitTime) + sizeof(sleepState), &checksum, sizeof(checksum)); // Adds checksum to the data array
    }

    /**
     * @brief Deserializes the Timer object from a data array and verifies checksum.
     * @param data The serialized data array.
     * @return True if deserialization is successful, false otherwise.
     */
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

    /**
     * @brief Calculates checksum for the given data array.
     * @param data The data array to calculate the checksum for.
     * @param length The length of the data array.
     * @return The calculated checksum.
     */
    uint8_t calculateChecksum(const uint8_t* data, size_t length) const {
        uint8_t checksum = 0;
        for (size_t i = 0; i < length; ++i) {
            checksum ^= data[i];
        }
        return checksum;
    }

private:
    time_t currentTime;        // Stores the current time
    uint16_t messageInterval;  // Stores the message interval
    uint16_t waitTime;         // Stores the wait time
    uint8_t sleepState;        // Stores the sleep state (2-bit value)
};

#endif // TIMER_H
