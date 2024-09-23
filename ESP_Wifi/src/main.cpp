#include <Arduino.h>
#include <WiFi.h>

// Access point credentials
const char* ssid = "ESP32-Test-AP";
const char* password = "12345678";

WiFiServer server(80);  // Create a TCP server on port 80

WiFiClient client; // Make client a global variable

unsigned long lastHeartbeatTime = 0;  // Stores the last time a heartbeat was received
const unsigned long heartbeatTimeout = 5000;  // 5 seconds timeout

String inputBuffer = ""; // Buffer to store incoming data

void setup() {
  Serial.begin(115200);

  // Initialize the WiFi in AP mode
  WiFi.softAP(ssid, password);

  // Start the TCP server
  server.begin();
  Serial.println("Server started");

  // Print the IP address of the ESP32 AP
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
}

void handleCommand(String command) {
  command.trim();        // Remove any leading/trailing whitespace
  command.toUpperCase(); // Convert to uppercase

  if (command == "HEARTBEAT") {
    // Update the heartbeat timestamp
    lastHeartbeatTime = millis();
    Serial.println("Heartbeat received.");
  } else if (command == "MANUAL") {
    Serial.println("Switching to MANUAL MODE...");
    client.println("ACK: MANUAL MODE activated.");
  } else if (command == "STOP") {
    Serial.println("STOP command received.");
    client.println("ACK: STOP command executed.");
  } else if (command == "AUTO") {
    Serial.println("Switching to AUTO MODE...");
    client.println("ACK: AUTO MODE activated.");
  } else if (command == "HOVER") {
    Serial.println("HOVER command received.");
    client.println("ACK: HOVER command executed.");
  } else {
    Serial.println("Unknown command received.");
    client.println("ERROR: Unknown command.");
  }
}

void loop() {
  if (!client || !client.connected()) {
    // Check for new client connection
    client = server.available();

    if (client) {
      Serial.println("Client connected");
      // Update lastHeartbeatTime when client connects
      lastHeartbeatTime = millis();
      inputBuffer = ""; // Clear the input buffer
    }
  } else {
    // Client is connected
    while (client.available()) {
      char c = client.read();
      if (c == '\n' || c == '\r') {
        if (inputBuffer.length() > 0) {
          handleCommand(inputBuffer);
          inputBuffer = ""; // Clear the buffer after handling the command
        }
      } else {
        inputBuffer += c;
      }
    }

    // Check for heartbeat timeout
    if (millis() - lastHeartbeatTime > heartbeatTimeout) {
      Serial.println("Heartbeat timeout! No heartbeat received.");
      // Handle heartbeat timeout (e.g., close client, reset variables, etc.)
      client.println("ERROR: Heartbeat timeout.");
      client.stop();
      Serial.println("Client disconnected");
      // Reset lastHeartbeatTime when client disconnects
      lastHeartbeatTime = 0;
    }
  }
}




