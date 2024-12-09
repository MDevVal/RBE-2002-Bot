#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>

#include "message.pb.h"
#include <Interface.h>
#include <ServerInterface.h>
#include <openmv.h>
#include <apriltagdatum.h>

const char *ssid = "RBE";
const char *password = "elm69wisest16poisoned";
const char *serverURL =
    "http://130.215.137.221:8080/nextState/"; // The server endpoint

Interface romiInterface = Interface(Serial);
ServerInterface server = ServerInterface(serverURL);

bool stateChange = false;
WiFiClient client;

OpenMV camera(Serial2);

uint8_t macHash() {
  uint8_t baseMac[6];
  esp_err_t ret = esp_wifi_get_mac(WIFI_IF_STA, baseMac);
  if (ret == ESP_OK) {
    return baseMac[3] ^ baseMac[4] ^ baseMac[5];
  } else {
    return 0;
  }
}

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  char fullURL[128];
  snprintf(fullURL, sizeof(fullURL), "%s%d", serverURL, macHash());
}

void loop() {
  AprilTagDatum tag;
  if(camera.checkUART(tag)) {
    message_AprilTag aprilTag = message_AprilTag_init_default;
    aprilTag.id = tag.id;
    aprilTag.has_pose = true;
    aprilTag.pose.x = tag.x;
    aprilTag.pose.y = tag.y;
    aprilTag.pose.z = tag.z;
    aprilTag.pose.roll = tag.roll;
    aprilTag.pose.pitch = tag.pitch;
    aprilTag.pose.heading = tag.yaw;

    romiInterface.sendProtobuf(aprilTag, message_AprilTag_fields, message_AprilTag_size);
  }


  size_t msg_size;
  if (!romiInterface.readUART(msg_size))
    return;

  message_RomiData data = message_RomiData_init_default;
  if (msg_size == message_RomiData_size) {

    // Decode the message from the Romi
    if (!romiInterface.readProtobuf(data, message_RomiData_fields))
      return;

    message_ServerCommand serverMessage = message_ServerCommand_init_default;

    // Send the Romi data to the server, and send the response back to the Romi
    if (server.HTTPRequest(data, serverMessage))
      romiInterface.sendProtobuf(serverMessage, message_ServerCommand_fields,
                                 message_ServerCommand_size);
  }
}
