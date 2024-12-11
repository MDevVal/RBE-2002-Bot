#pragma once
#include <HTTPClient.h>
#include <Arduino.h>

#include <pb_encode.h>
#include <pb_decode.h>
#include "message.pb.h" 

class ServerInterface
{
public:

    ServerInterface(char* server) : URL(server) {}

    bool HTTPRequest(const message_RomiData &sendMsg, message_ServerCommand &recMsg) {
        bool ret = false;

        // Serialize the protobuf message
        uint8_t buffer[message_RomiData_size];  // Buffer to hold the serialized message
        pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));

        if (!pb_encode(&stream, message_RomiData_fields, &sendMsg)) {
            Serial.print("Encoding failed: ");
            return false;
        }

        // Send the serialized protobuf via HTTP POST
        HTTPClient http;
        // Serial.println("doing a thing");
        // Serial.println(URL);
        http.begin(URL);  // The server endpoint

        // Set content type to application/x-protobuf
        // http.addHeader("Content-Type", "application/x-protobuf");

        // Send the POST request with the protobuf data
        int httpCode = http.POST(buffer, stream.bytes_written);

        if (httpCode == HTTP_CODE_OK) {

            Serial.println("Protobuf sent successfully. Reading...");
            
            int len = http.getSize();
            uint8_t* buffer = new uint8_t[len];

            // Get the data from the server
            http.getStream().readBytes(buffer, len);

            // Parse the protobuf
            pb_istream_t stream = pb_istream_from_buffer(buffer, len);
            if (!pb_decode(&stream, message_ServerCommand_fields, &recMsg)) {
                Serial.print("Error decoding protobuf");
            } else {
                Serial.print("State Change: ");
                Serial.println(recMsg.state);
                Serial.print("Has cell data: ");
                Serial.println(recMsg.has_targetGridCell);
                Serial.print("Base speed: ");
                Serial.println(recMsg.baseSpeed);
                ret = true;
            }

            delete[] buffer;

        } else {
            Serial.print("Error communicating with server");
            Serial.println(httpCode);
        }

        http.end();  // Clean up the HTTP request
        return ret;
    }

    void setServerURL(char* newURL) {
        Serial.println("Setting new server URL");
        Serial.println(newURL);
        URL = newURL;
    }

private:
    char* URL;
};
