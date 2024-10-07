#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

// Define UART Pins
#define TXD_PIN (GPIO_NUM_17) // ESP32 TX Pin connected to FC RX
#define RXD_PIN (GPIO_NUM_16) // ESP32 RX Pin connected to FC TX

// Define CRSF Constants
#define CRSF_SYNC_BYTE 0xC8

// Define Wi-Fi Credentials
const char* ssid = "ESP32_Channel_Control";
const char* password = "12345678";

// Initialize Web Server and WebSocket
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Define Channel Values (Assuming 16 channels)
uint16_t channels[16];

// Define Payload and CRSF Frame Buffers
uint8_t payload[22];
uint8_t crsfFrame[26];

// Control Variables for Channels
float roll = 0.0;     // Channel 1
float pitch = 0.0;    // Channel 2
float throttle = 0.0; // Channel 3
float yaw = 0.0;      // Channel 4
bool isArmed = false; // Channel 5 (Aux 1)

// Telemetry Variables
float batteryVoltage = 0.0;
float batteryCurrent = 0.0;
uint8_t rssi = 0;

// Accelerometer Data Variables
float accRoll = 0.0;
float accPitch = 0.0;
float accYaw = 0.0;

// Barometer Data
float baroAltitude = 0.0;
float verticalSpeed = 0.0;

// Flight Mode
String flightMode = "Unknown";

// Vibration Levels
float vibrationX = 0.0;
float vibrationY = 0.0;
float vibrationZ = 0.0;

// Motor Outputs
uint16_t motorOutputs[8] = {0}; // Initialize motor outputs (Adjust size as needed)

// Temperature Sensors
float fcTemperature = 0.0;

// Warning Messages
String warningMessage = "None";

// Function Prototypes
void initializeChannels();
void updateChannels();
void packChannels();
uint8_t crc8(const uint8_t *data, uint8_t len);
void buildCrsfFrame();
void parseTelemetryPacket(uint8_t *buffer, uint8_t length);
void readTelemetry();
void sendTelemetry();
void onWebSocketEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type,
                      void * arg, uint8_t * data, size_t len);

// Initialize Channels with Neutral Values
void initializeChannels() {
  for (int i = 0; i < 16; i++) {
    channels[i] = 992; // Neutral value for all channels
  }
}

// Update Channel Values Based on Control Inputs
void updateChannels() {
  channels[0] = constrain((int)(992 + (roll * 820)), 172, 1811);      // Roll (Channel 1)
  channels[1] = constrain((int)(992 + (pitch * 820)), 172, 1811);     // Pitch (Channel 2)
  channels[2] = constrain((int)(50 + (throttle * 1761)), 0, 1811); // Throttle (Channel 3)
  channels[3] = constrain((int)(992 + (yaw * 820)), 172, 1811);       // Yaw (Channel 4)

  // Channel 5 for Arm/Disarm (Aux 1)
  channels[4] = isArmed ? 1811 : 172; // High value for armed, low for disarmed
}

// Pack Channel Values into CRSF Payload
void packChannels() {
  uint8_t payloadIndex = 0;
  uint32_t bitBuffer = 0;
  uint8_t bitsInBuffer = 0;

  memset(payload, 0, sizeof(payload));

  for (uint8_t i = 0; i < 16; i++) {
    bitBuffer |= ((uint32_t)(channels[i] & 0x07FF)) << bitsInBuffer;
    bitsInBuffer += 11;

    while (bitsInBuffer >= 8) {
      payload[payloadIndex++] = bitBuffer & 0xFF;
      bitBuffer >>= 8;
      bitsInBuffer -= 8;
    }
  }

  if (payloadIndex < 22) {
    payload[payloadIndex] = bitBuffer & 0xFF;
  }
}

// Calculate CRC8 for CRSF Frame
uint8_t crc8(const uint8_t *data, uint8_t len) {
  uint8_t crc = 0;
  while (len--) {
    crc ^= *data++;
    for (uint8_t i = 0; i < 8; i++) {
      if (crc & 0x80)
        crc = (crc << 1) ^ 0xD5; // Correct polynomial for CRSF
      else
        crc <<= 1;
    }
  }
  return crc;
}

// Build CRSF Frame with Payload and CRC
void buildCrsfFrame() {
  crsfFrame[0] = CRSF_SYNC_BYTE; // Start of Frame
  crsfFrame[1] = 24;            // Length
  crsfFrame[2] = 0x16;          // Type (e.g., Binding, Channel Data)

  memcpy(&crsfFrame[3], payload, 22); // Copy Payload

  crsfFrame[25] = crc8(&crsfFrame[2], 23); // Compute CRC8
}

// Parse Telemetry Packet from FC
void parseTelemetryPacket(uint8_t *buffer, uint8_t length) {
  uint8_t type = buffer[0];
  uint8_t *payload = &buffer[1];
  uint8_t payloadLength = length - 1;

  switch (type) {
    case 0x07: // Link statistics
      if (payloadLength >= 10) {
        rssi = payload[0]; // RSSI in dBm
        // Additional link statistics can be parsed here if needed
      }
      break;
    case 0x08: // Battery voltage and current
      if (payloadLength >= 4) {
        batteryVoltage = (payload[0] << 8 | payload[1]) / 100.0; // Voltage in Volts
        batteryCurrent = (payload[2] << 8 | payload[3]) / 100.0; // Current in Amps
      }
      break;
    case 0x1E: // Attitude (Roll, Pitch, Yaw)
      if (payloadLength >= 6) {
        int16_t rawRoll = (int16_t)(payload[0] << 8 | payload[1]);
        int16_t rawPitch = (int16_t)(payload[2] << 8 | payload[3]);
        int16_t rawYaw = (int16_t)(payload[4] << 8 | payload[5]);

        // Convert to degrees (assuming values are in centidegrees)
        accRoll = rawRoll / 100.0;
        accPitch = rawPitch / 100.0;
        accYaw = rawYaw / 100.0;
      }
      break;
    case 0x10: // Barometer data
      if (payloadLength >= 6) {
        int32_t rawAltitude = (int32_t)(payload[0] << 24 | payload[1] << 16 | payload[2] << 8 | payload[3]);
        int16_t rawVSpeed = (int16_t)(payload[4] << 8 | payload[5]);

        baroAltitude = rawAltitude / 100.0;     // Convert cm to meters
        verticalSpeed = rawVSpeed / 100.0;      // Convert cm/s to m/s
      }
      break;
    case 0x21: // Flight mode
      // Flight mode is sent as a variable length string
      flightMode = "";
      for (uint8_t i = 0; i < payloadLength; i++) {
        flightMode += (char)payload[i];
      }
      break;
    case 0x26: // Motor outputs
      {
        uint8_t numMotors = payloadLength / 2; // Each motor output is 2 bytes
        for (uint8_t i = 0; i < numMotors && i < 8; i++) {
          motorOutputs[i] = (uint16_t)(payload[2*i] << 8 | payload[2*i + 1]);
        }
      }
      break;
    case 0x0E: // Temperature sensors
      if (payloadLength >= 2) {
        int16_t rawTemp = (int16_t)(payload[0] << 8 | payload[1]);
        fcTemperature = rawTemp / 100.0; // Temperature in degrees Celsius
      }
      break;
    case 0x22: // Warning messages
      // Warning message is sent as a variable length string
      warningMessage = "";
      for (uint8_t i = 0; i < payloadLength; i++) {
        warningMessage += (char)payload[i];
      }
      break;
    case 0x30: // Vibration levels (Assuming custom message type 0x30)
      if (payloadLength >= 6) {
        int16_t rawVibX = (int16_t)(payload[0] << 8 | payload[1]);
        int16_t rawVibY = (int16_t)(payload[2] << 8 | payload[3]);
        int16_t rawVibZ = (int16_t)(payload[4] << 8 | payload[5]);

        // Convert to g's or appropriate units based on FC output
        vibrationX = rawVibX / 1000.0;
        vibrationY = rawVibY / 1000.0;
        vibrationZ = rawVibZ / 1000.0;
      }
      break;
    default:
      // Unknown telemetry type
      break;
  }
}

// Read Telemetry Data from FC via UART2
void readTelemetry() {
  static uint8_t buffer[64];
  static uint8_t bufferIndex = 0;

  while (Serial2.available()) {
    uint8_t byte = Serial2.read();

    // Check for start of packet (Sync Byte)
    if (byte == CRSF_SYNC_BYTE) {
      bufferIndex = 0;
      buffer[bufferIndex++] = byte;
    } else if (bufferIndex > 0) {
      buffer[bufferIndex++] = byte;

      // Check if we have at least the length byte
      if (bufferIndex == 2) {
        // buffer[1] is the length
        if (buffer[1] > sizeof(buffer) - 2) {
          // Invalid length, reset buffer
          bufferIndex = 0;
        }
      } else if (bufferIndex == buffer[1] + 2) {
        // We have the full packet
        uint8_t length = buffer[1];
        uint8_t calculatedCrc = crc8(&buffer[2], length - 1);
        uint8_t receivedCrc = buffer[bufferIndex - 1];

        if (calculatedCrc == receivedCrc) {
          // Valid packet, parse telemetry
          parseTelemetryPacket(&buffer[2], length - 1);
        }
        // Reset buffer for next packet
        bufferIndex = 0;
      }
    }
  }
}

// Send Telemetry Data via WebSocket
void sendTelemetry() {
  // Use StaticJsonDocument for stack allocation with a fixed size
  StaticJsonDocument<1024> jsonDoc; // Adjust the size as needed

  // Populate the JSON document with telemetry data
  jsonDoc["voltage"] = batteryVoltage;
  jsonDoc["current"] = batteryCurrent;
  jsonDoc["rssi"] = rssi;

  // Accelerometer data
  jsonDoc["accRoll"] = accRoll;
  jsonDoc["accPitch"] = accPitch;
  jsonDoc["accYaw"] = accYaw;

  // Barometer data
  jsonDoc["baroAltitude"] = baroAltitude;
  jsonDoc["verticalSpeed"] = verticalSpeed;

  // Flight mode
  jsonDoc["flightMode"] = flightMode;

  // Vibration levels
  jsonDoc["vibrationX"] = vibrationX;
  jsonDoc["vibrationY"] = vibrationY;
  jsonDoc["vibrationZ"] = vibrationZ;

  // Motor outputs
  JsonArray motorArray = jsonDoc["motorOutputs"].to<JsonArray>();
  for (uint8_t i = 0; i < 8; i++) { // Adjust based on the number of motors
    motorArray.add(motorOutputs[i]);
  }

  // Temperature sensors
  jsonDoc["fcTemperature"] = fcTemperature;

  // Warning messages
  jsonDoc["warningMessage"] = warningMessage;

  // Serialize the JSON document to a string
  String jsonString;
  serializeJson(jsonDoc, jsonString);

  // Send the JSON string over WebSocket to all connected clients
  ws.textAll(jsonString);
}

// Handle WebSocket Events
void onWebSocketEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type,
                      void * arg, uint8_t * data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.println("WebSocket client connected");
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.println("WebSocket client disconnected");
  }
}

// Setup Function
void setup() {
  // Initialize Serial for debugging
  Serial.begin(115200);
  
  // Initialize UART2 for CRSF communication
  Serial2.begin(420000, SERIAL_8N1, RXD_PIN, TXD_PIN); // High baud rate for CRSF

  // Initialize Channel Values
  initializeChannels();

  // Initialize Wi-Fi as Access Point
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  // Initialize LittleFS for File System
  if (!LittleFS.begin()) {
    Serial.println("An error has occurred while mounting LittleFS");
    return;
  }

  // Serve the Web Page from LittleFS
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/index.html", "text/html");
  });

  // Handle Control Updates via HTTP GET Requests
  server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request){
    String inputMessage;

    // Handle Roll Update
    if (request->hasParam("roll")) {
      inputMessage = request->getParam("roll")->value();
      roll = inputMessage.toFloat() / 100.0;
      Serial.println("Roll set to " + String(roll * 100) + "%");
    }

    // Handle Pitch Update
    if (request->hasParam("pitch")) {
      inputMessage = request->getParam("pitch")->value();
      pitch = inputMessage.toFloat() / 100.0;
      Serial.println("Pitch set to " + String(pitch * 100) + "%");
    }

    // Handle Yaw Update
    if (request->hasParam("yaw")) {
      inputMessage = request->getParam("yaw")->value();
      yaw = inputMessage.toFloat() / 100.0;
      Serial.println("Yaw set to " + String(yaw * 100) + "%");
    }

    // Handle Throttle Update
    if (request->hasParam("throttle")) {
      inputMessage = request->getParam("throttle")->value();
      throttle = inputMessage.toFloat() / 100.0;
      Serial.println("Throttle set to " + String(throttle * 100) + "%");
    }

    // Handle Arm/Disarm Update
    if (request->hasParam("arm")) {
      inputMessage = request->getParam("arm")->value();
      isArmed = (inputMessage == "1");
      Serial.println(isArmed ? "Armed" : "Disarmed");
    }

    // Handle Stop Motors Command
    if (request->hasParam("stop")) {
      // Stop motors by setting throttle to zero and disarming
      throttle = 0.0;
      isArmed = false;

      // Reset Roll, Pitch, and Yaw to zero
      roll = 0.0;
      pitch = 0.0;
      yaw = 0.0;

      Serial.println("Motors stopped and controls reset");
    }

    // Respond with No Content
    request->send(204);
  });

  // Attach WebSocket Event Handler
  ws.onEvent(onWebSocketEvent);
  server.addHandler(&ws);

  // Start the Web Server
  server.begin();
}

// Global Variable to Track Telemetry Sending Interval
unsigned long lastTelemetryTime = 0;

// Main Loop Function
void loop() {
  // // Read Telemetry Data from FC
  // readTelemetry();

  // // Check if it's time to send telemetry data
  // unsigned long currentTime = millis();
  // if (currentTime - lastTelemetryTime >= 500) { // Send every 500 ms
  //   sendTelemetry();
  //   lastTelemetryTime = currentTime;
  // }

  // Update Channel Values Based on Controls
  updateChannels();

  // Pack Channel Values into CRSF Payload
  packChannels();

  // Build CRSF Frame with Payload and CRC
  buildCrsfFrame();

  // Send CRSF Frame to FC via UART2
  Serial2.write(crsfFrame, 26); // CRSF frame size is 26 bytes

  // Clean Up Disconnected WebSocket Clients
  ws.cleanupClients();

  // Delay to Maintain Update Rate (50Hz)
  delay(30); // 20 ms delay
}
