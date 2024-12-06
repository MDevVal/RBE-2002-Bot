#include <Arduino.h>
#include <WiFi.h>

#include <pb_decode.h>
#include <pb_encode.h>
#include "message.pb.h"
#include <Interface.h>
#include <ServerInterface.h>

const char* ssid = "RBE";
const char* password = "elm69wisest16poisoned";
const char* serverURL = "http://130.215.137.221:8080/protobuf";  // The server endpoint

Interface romiInterface = Interface(Serial1);
ServerInterface server = ServerInterface(serverURL);

bool stateChange = false;
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

  
}

void loop() {
  size_t msg_size;
  if (!romiInterface.readUART(msg_size)) return;

  message_RomiData data = message_RomiData_init_default;
  if (msg_size == message_RomiData_size) {
      if (!romiInterface.readProtobuf(data, message_RomiData_fields)) return; 

      // Send Protobuf over HTTP
      server.sendHTTP(data);

      message_ServerCommand serverMessage = message_ServerCommand_init_zero;
      if (server.requestHTTP(serverMessage)) romiInterface.sendProtobuf(serverMessage, message_ServerCommand_fields, message_ServerCommand_size);
  }
}

