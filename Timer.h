#ifndef TIMER_H // Prevents multiple inclusions of this header file
#define TIMER_H

#include <ctime> // Includes the C time library for time-related functions
#include <Arduino.h> // Includes the Arduino core library

// Timer class definition
class Timer {
public:
    // Constructor that initializes the Timer object with provided values
    Timer(time_t currentTime, uint16_t messageInterval, uint16_t waitTime, uint8_t sleepState)
        : currentTime(currentTime), messageInterval(messageInterval), waitTime(waitTime), sleepState(sleepState) {}

    // Constructor that initializes the Timer object from a serialized data array
    Timer(const uint8_t* data) {
        deserialize(data);
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

    // Serializes the Timer object into a data array
    void serialize(uint8_t* data) const {
        memcpy(data, &currentTime, sizeof(currentTime)); // Copies currentTime to the data array
        memcpy(data + sizeof(currentTime), &messageInterval, sizeof(messageInterval)); // Copies messageInterval to the data array
        memcpy(data + sizeof(currentTime) + sizeof(messageInterval), &waitTime, sizeof(waitTime)); // Copies waitTime to the data array
        memcpy(data + sizeof(currentTime) + sizeof(messageInterval) + sizeof(waitTime), &sleepState, sizeof(sleepState)); // Copies sleepState to the data array
    }

    // Deserializes the Timer object from a data array
    void deserialize(const uint8_t* data) {
        memcpy(&currentTime, data, sizeof(currentTime)); // Copies currentTime from the data array
        memcpy(&messageInterval, data + sizeof(currentTime), sizeof(messageInterval)); // Copies messageInterval from the data array
        memcpy(&waitTime, data + sizeof(currentTime) + sizeof(messageInterval), sizeof(waitTime)); // Copies waitTime from the data array
        memcpy(&sleepState, data + sizeof(currentTime) + sizeof(messageInterval) + sizeof(waitTime), sizeof(sleepState)); // Copies sleepState from the data array
    }

private:
    time_t currentTime; // Stores the current time
    uint16_t messageInterval; // Stores the message interval
    uint16_t waitTime; // Stores the wait time
    uint8_t sleepState; // Stores the sleep state
};

#endif // TIMER_H
