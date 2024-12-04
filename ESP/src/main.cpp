#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <pb_decode.h>
#include "message.pb.h"  // The generated Nanopb header file

#include "../ESP-ANDY.psk.h"

const char* ssid = "WPI-PSK";
const char* password = PASSWORD;
const char* serverURL = "http://10.0.0.1:8080/protobuf";  // The server endpoint

WiFiClient client;

void setup() {
  Serial.begin(115200);
  
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Make HTTP request to the server
  HTTPClient http;
  http.begin(serverURL);

  int httpCode = http.GET();  // Send GET request

  if (httpCode == HTTP_CODE_OK) {
    int len = http.getSize();
    uint8_t* buffer = new uint8_t[len];

    // Get the data from the server
    http.getStream().readBytes(buffer, len);

    // Now, let's parse the protobuf data
    message_ServerCommand message = message_ServerCommand_init_zero;

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
    }

    delete[] buffer;
  } else {
    Serial.println("Failed to fetch data from server.");
  }

  http.end();  // Clean up the HTTP request
}

void loop() {
  // No need to do anything in loop for this example
}
