/**
 * @file WakeUpCoordination.h
 * @brief This file contains the WakeUpCoordination class, which coordinates wake-up and sleep cycles for LoRa communication between a host and client device.
 */

#ifndef WAKE_UP_COORDINATION_H
#define WAKE_UP_COORDINATION_H

#include <Arduino.h>
#include <RadioLib.h>
#include "Timer.h"

/**
 * @class WakeUpCoordination
 * @brief Manages the wake-up coordination between host and client devices using LoRa communication.
 */
class WakeUpCoordination {
public:
    /**
     * @brief Constructor for WakeUpCoordination.
     */
    WakeUpCoordination() {}

    /**
     * @brief Coordinates the wake-up and sleep cycles.
     * @param timer Timer object to manage timing.
     * @param isHost Boolean indicating if the device is a host.
     * @param radio LoRa radio object.
     * @param ledFunction Function pointer to control the LED.
     * @param displayFunction Function pointer to control the display.
     * @return Sleep duration in seconds.
     */
    uint32_t coordinate(Timer& timer, bool isHost, SX1262& radio, void (*ledFunction)(int), void (*displayFunction)(const String&, const String&)) {
        _ledFunction = ledFunction;
        _displayFunction = displayFunction;
        if (isHost) {
            return hostCoordinate(timer, radio);
        } else {
            return clientCoordinate(timer, radio);
        }
    }

private:
    static const size_t DATA_SIZE = sizeof(time_t) + 2 * sizeof(uint16_t) + sizeof(uint8_t) + sizeof(uint8_t); // Data size for serialization
    void (*_ledFunction)(int); // Function pointer for LED control
    void (*_displayFunction)(const String&, const String&); // Function pointer for display control

    /**
     * @struct SentMessageInfo
     * @brief Structure to hold the send time and checksum.
     */
    struct SentMessageInfo {
        unsigned long sendTime; // Send time of the message
        uint8_t checksum;       // Checksum of the sent message
    };

    SentMessageInfo lastSentMessage; // Instance of SentMessageInfo

    /**
     * @brief Coordinates the host operations.
     * @param timer Timer object to manage timing.
     * @param radio LoRa radio object.
     * @return Sleep duration in seconds.
     */
    uint32_t hostCoordinate(Timer& timer, SX1262& radio) {
        uint8_t data[DATA_SIZE];
        uint8_t receivedData[DATA_SIZE];
        unsigned long lastSendTime = 0;

        while (true) {
            if (millis() - lastSendTime >= 1100) {
                time_t currentTime = time(NULL);
                uint16_t messageInterval = 10;  // 10 second message interval
                uint16_t waitTime = 5;
                uint8_t sleepState = 1;

                timer = Timer(currentTime, messageInterval, waitTime, sleepState);
                timer.serialize(data);

                _ledFunction(20); // LED on
                int state = radio.transmit(data, sizeof(data));
                _ledFunction(0); // LED off

                if (state == RADIOLIB_ERR_NONE) {
                    Serial.println("Host sent Timer object successfully.");
                } else {
                    Serial.println("Host failed to send Timer object.");
                }

                lastSentMessage.sendTime = millis();
                lastSentMessage.checksum = data[DATA_SIZE - 1];

                lastSendTime = millis();
            }

            int state = radio.receive(receivedData, sizeof(receivedData));
            if (state == RADIOLIB_ERR_NONE) {
                uint8_t receivedChecksum = receivedData[0];
                if (receivedChecksum == lastSentMessage.checksum) {
                    unsigned long receivedTime = millis();
                    unsigned long timeSinceSent = (receivedTime - lastSentMessage.sendTime) / 1000;
                    unsigned long sleepDuration = timer.getMessageInterval() - timeSinceSent;

                    Serial.print("Host received confirmation checksum. Time since sent: ");
                    Serial.print(timeSinceSent);
                    Serial.print(" seconds. Sleeping for: ");
                    Serial.print(sleepDuration);
                    Serial.println(" seconds.");

                    return sleepDuration;
                }
            }

            delay(1); // Small delay to prevent tight loop
        }
    }

    /**
     * @brief Coordinates the client operations.
     * @param timer Timer object to manage timing.
     * @param radio LoRa radio object.
     * @return Sleep duration in seconds.
     */
    uint32_t clientCoordinate(Timer& timer, SX1262& radio) {
        uint8_t receivedData[DATA_SIZE];
        unsigned long receivedTime = 0;

        while (true) {
            radio.startReceive();
            int state = radio.receive(receivedData, sizeof(receivedData));
            if (state == RADIOLIB_ERR_NONE) {
                Serial.println("Client received data.");

                Timer receivedTimer(receivedData);

                uint8_t receivedChecksum = receivedData[DATA_SIZE - 1];
                uint8_t calculatedChecksum = receivedTimer.calculateChecksum(receivedData, DATA_SIZE - 1);

                if (receivedChecksum == calculatedChecksum) {
                    _displayFunction("Received Timer:", "Msg Interval: " + String(receivedTimer.getMessageInterval()) + " sec");
                    _displayFunction("Checksum: " + String(receivedChecksum, HEX), "Calculated: " + String(calculatedChecksum, HEX));

                    timer = receivedTimer;
                    receivedTime = millis();

                    Serial.println("Client received valid Timer object.");

                    for (int i = 0; i < 5; ++i) {
                        delay(225);
                        uint8_t checksumData[1] = {receivedChecksum};
                        int sendState = radio.transmit(checksumData, sizeof(checksumData));
                        if (sendState == RADIOLIB_ERR_NONE) {
                            Serial.println("Client sent checksum successfully.");
                        } else {
                            Serial.println("Client failed to send checksum.");
                        }
                    }

                    unsigned long timeElapsed = (millis() - receivedTime) / 1000;
                    unsigned long meetingInterval = timer.getMessageInterval() - timeElapsed;

                    Serial.print("Client going to sleep for ");
                    Serial.print(meetingInterval);
                    Serial.println(" seconds.");
                    return meetingInterval;
                } else {
                    Serial.println("Received data checksum mismatch.");
                }
            } else {
                Serial.println("Client did not receive data.");
            }

            delay(1); // Small delay to prevent tight loop
        }
    }
};

#endif // WAKE_UP_COORDINATION_H
