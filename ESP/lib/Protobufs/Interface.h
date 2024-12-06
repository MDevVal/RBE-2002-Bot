#include <Arduino.h>
// #include <Wire.h>
// #include <SPI.h>
#include <pb_encode.h>
#include <pb_decode.h>

class Interface
{
public:
    Interface(HardwareSerial& port) : port(port) {
        port.begin(115200);
    }

    template <typename T>
    void sendProtobuf(const T& msg, const pb_msgdesc_t* fields, size_t size) {
        uint8_t buffer[size];  // Buffer to hold the serialized message
        pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));

        // Serial.println(sizeof(T));

        // Serialize the message to the buffer
        if (!pb_encode(&stream, fields, &msg)) {
            Serial.println("Serialization failed!");
            return; 
        }

        // Serial.println(stream.bytes_written);
        // Serial.println((byte) size);

        // Send the serialized data over Serial
        port.write(0xAA);  // Start byte
        // port.write(0xBB);  // Start byte
        // port.write(0xCC);  // Start byte
        // port.write(0xDD);  // Start byte
        // Serial.println(sizeof(float));
        // Serial.println(sizeof(bool));
        port.write((byte) size);  // Message length
        port.write((byte) stream.bytes_written);  // Buffer length
        port.write(buffer, stream.bytes_written);
        // port.println();  // Add a newline for better separation in the log
    }

    bool readUART(size_t& msg_size) {
        // Check if there is any data available on the Serial interface
        if (port.available() > 0) {
            // uint8_t buf[4]; port.readBytes(buf, 4);
            // uint8_t chk[4] = {0xDD, 0xCC, 0xBB, 0xAA}; 
            // if (buf != chk) {
            if (port.read() != 0xAA) {
                return false;
            }
            
            msg_size = port.peek();
            // Serial.println(msg_size);
            return true;
        }

        return false;
    }

    template <typename T>
    bool readProtobuf(T& message, const pb_msgdesc_t* fields) {

        size_t msg_size = port.read();
        if (msg_size != sizeof(T)) {
            // Serial.println("Something went very wrong!");
            // return false;
        }

        size_t buffer_size = port.read(); // Serial transmits start bit -> msg_size -> buffer_size

        uint8_t buffer[buffer_size];
        if (port.available() < buffer_size) {
            // Serial.println("Not enough bytes available");
            return false;
        }

        if (!port.readBytes(buffer, buffer_size));//Serial.println("ahhh buffer bad");

        // // Decode and handle ChassisProto_ChassisMsg
        pb_istream_t stream = pb_istream_from_buffer(buffer, buffer_size);

        if (!pb_decode(&stream, fields, &message)) {
            // Serial.println("Failed to decode " + String(buffer_size) + " bytes for message of size " + String(msg_size));
            return false;
        } else {
            // Serial.println("aahhh");
            // Serial.println("Successfully decoded " + String(buffer_size) + "bytes for message of size " + String(msg_size));
            return true;
        }
    }

private:
    HardwareSerial& port;
};