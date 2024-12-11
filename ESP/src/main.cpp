#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>

#include "message.pb.h"
#include <Interface.h>
#include <ServerInterface.h>
#include <apriltagdatum.h>
#include <openmv.h>

const char *ssid = "RBE";
const char *password = "elm69wisest16poisoned";
char *serverURL =
    // "http://130.215.137.221:8080/nextState/178"; // The server endpoint
    "http://130.215.137.221:8080/protobuf";

Interface romiInterface = Interface(Serial1);
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
  Serial1.begin(115200, SERIAL_8N1, 25, 26);
  Serial2.begin(115200);

  delay(5000);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  char fullURL[128];
  snprintf(fullURL, sizeof(fullURL), "%s%d", serverURL, macHash());
  Serial.println(fullURL);
  // server.setServerURL(fullURL);
}

void loop() {

  message_ServerCommand serverMessage = message_ServerCommand_init_default;

  // Send the Romi data to the server, and send the response back to the
  if (server.HTTPRequest(message_RomiData_init_default, serverMessage)) {
    Serial.println("recived message from server, sending to romi");
    romiInterface.sendProtobuf(serverMessage, message_ServerCommand_fields,
                                message_ServerCommand_size);
  }

  // AprilTagDatum tag;
  // if (camera.checkUART(tag)) {
  //   Serial.println("Tag ID: " + String(tag.id));

  //   message_AprilTag aprilTag = message_AprilTag_init_default;
  //   // aprilTag.id = tag.id;
  //   // aprilTag.has_pose = true;
  //   // aprilTag.pose.x = tag.x;
  //   // aprilTag.pose.y = tag.y;
  //   // aprilTag.pose.z = tag.z;
  //   // aprilTag.pose.roll = tag.roll;
  //   // aprilTag.pose.pitch = tag.pitch;
  //   // aprilTag.pose.heading = tag.yaw;

  //   aprilTag.id = tag.id;
  //   aprilTag.cx = tag.cx;
  //   aprilTag.cy = tag.cy;
  //   aprilTag.w = tag.w;
  //   aprilTag.h = tag.h;
  //   aprilTag.rot = tag.rot;

  //   romiInterface.sendProtobuf(aprilTag, message_AprilTag_fields,
  //                              message_AprilTag_size);
  // }

  // size_t msg_size;
  // if (!romiInterface.readUART(msg_size))
  //   return;

  // message_RomiData data = message_RomiData_init_default;
  // if (msg_size == message_RomiData_size) {

  //   // Decode the message from the Romi
  //   if (!romiInterface.readProtobuf(data, message_RomiData_fields)) 
  //     return;
  //   Serial.println("got message from romi");

      // message_ServerCommand serverMessage = message_ServerCommand_init_default;

      // Send the Romi data to the server, and send the response back to the
      // if (server.HTTPRequest(data, serverMessage)) {
      //   Serial.println("recived message from server, sending to romi");
      //   romiInterface.sendProtobuf(serverMessage, message_ServerCommand_fields,
      //                               message_ServerCommand_size);
      // }
  // }
}
