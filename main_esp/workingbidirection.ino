#include <WiFi.h>
#include <AsyncTCP.h>

// WiFi credentials
const char* ssid = "iPhone";             // Your WiFi SSID
const char* password = "12345678";       // Your WiFi Password

// Static IP configuration (optional)
IPAddress local_IP(172, 20, 10, 13);    // Set a static IP for the ESP32
IPAddress gateway(192, 168, 1, 1);       // Gateway (your router)
IPAddress subnet(255, 255, 255, 0);      // Subnet mask

// Ultrasonic sensor pin definitions
#define TRIG_PIN 18
#define ECHO_PIN 19

// TCP server configuration
WiFiServer tcpServer(12345);  // Create a server on port 12345
WiFiClient client;            // Client to communicate with

// Function to initialize WiFi and optionally set a static IP
void setupWiFi() {
  // Optionally configure static IP
  if (!WiFi.config(local_IP, gateway, subnet)) {
    Serial.println("STA Failed to configure");
  }

  WiFi.begin(ssid, password); // Connect to WiFi

  // Wait until connected
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Display the assigned IP address
  Serial.println("");
  Serial.print("Connected to WiFi. IP address: ");
  Serial.println(WiFi.localIP());
}

long measureDistance() {
  digitalWrite(TRIG_PIN, LOW);  // Ensure trigger pin is low
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);  // Send trigger pulse
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Read the echo pulse duration
  long duration = pulseIn(ECHO_PIN, HIGH);
  long distance = duration * 0.034 / 2;  // Calculate distance in cm

  return distance;
}

void setup() {
  // Initialize serial communication for debugging
  Serial.begin(115200);

  // Initialize ultrasonic sensor pins
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Initialize WiFi
  setupWiFi();

  // Start TCP server
  tcpServer.begin();
  Serial.println("TCP server started.");
}

void loop() {
  // Check for incoming client connections
  if (!client || !client.connected()) {
    client = tcpServer.available();  // Accept new client
    if (client) {
      Serial.println("Client connected.");
    }
  }

  // If a client is connected, measure and send distance
  if (client && client.connected()) {
    long distance = measureDistance();  // Get ultrasonic sensor reading

    // Send distance to client
    String message = "Distance: " + String(distance) + " cm\n";
    client.print(message);
    Serial.println("Sent to client: " + message);

    // Check for incoming commands from the client
    if (client.available()) {
      String command = client.readStringUntil('\n'); // Read the command until newline
      Serial.print("Received command: ");
      Serial.println(command); // Print the received command
    }

    // Delay before sending next reading
    delay(1000);  // Send reading every second
  }
}