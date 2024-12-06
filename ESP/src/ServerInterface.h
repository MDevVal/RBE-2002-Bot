#pragma once
#include <HTTPClient.h>
#include <Arduino.h>

#include <pb_encode.h>
#include <pb_decode.h>
#include "message.pb.h" 

class ServerInterface
{
public:
    ServerInterface(const char* serverURL) : serverURL(serverURL) {}

    bool requestHTTP(message_ServerCommand &message) {
        bool ret = false;
        
        // Make HTTP request to the server
        HTTPClient http;
        http.begin(serverURL);

        int httpCode = http.GET();  // Send GET request

        if (httpCode == HTTP_CODE_OK) {
            int len = http.getSize();
            uint8_t* buffer = new uint8_t[len];

            // Get the data from the server
            http.getStream().readBytes(buffer, len);

            // Parse the protobuf
            pb_istream_t stream = pb_istream_from_buffer(buffer, len);
            if (!pb_decode(&stream, message_ServerCommand_fields, &message)) {
            Serial.print("Error decoding protobuf");
            } else {
            Serial.print("State Change: ");
            Serial.println(message.stateChange.state);
            Serial.print("Has cell data: ");
            Serial.println(message.stateChange.has_targetGridCell);
            Serial.print("Base speed: ");
            Serial.println(message.stateChange.baseSpeed);
            ret = true;
            }

            delete[] buffer;
        } else {
            Serial.println("Failed to fetch data from server.");
        }

        http.end();  // Clean up the HTTP request
        return ret;
    }


    bool sendHTTP(const message_RomiData &message) {
        // Serialize the protobuf message
        uint8_t buffer[message_RomiData_size];  // Buffer to hold the serialized message
        pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));

        if (!pb_encode(&stream, message_RomiData_fields, &message)) {
            Serial.print("Encoding failed: ");
            return false;
        }

        // Send the serialized protobuf via HTTP POST
        HTTPClient http;
        http.begin(serverURL);  // The server endpoint

        // Set content type to application/x-protobuf
        http.addHeader("Content-Type", "application/x-protobuf");

        // Send the POST request with the protobuf data
        int httpCode = http.POST(buffer, stream.bytes_written);

        if (httpCode == HTTP_CODE_OK) {
            Serial.println("Protobuf sent successfully");
        } else {
            Serial.print("Error sending protobuf: ");
            Serial.println(httpCode);
        }

        http.end();  // Clean up the HTTP request
        return (httpCode == HTTP_CODE_OK);
    }
private:
    const char* serverURL;
};