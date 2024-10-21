#include "esp_camera.h"
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// Camera model selection
#define CAMERA_MODEL_AI_THINKER

// WiFi credentials
const char* ssid = "iPhone";
const char* password = "12345678";

// TCP socket server credentials
const char* socket_host = "192.168.1.10";  // Replace with the socket server's IP address
const int socket_port = 1234;              // Replace with the socket server's port

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

WiFiClient socketClient;

// Camera configuration for AI-Thinker
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// Function to start the camera server
void startCameraServer();

// Function to handle socket communication
void handleSocketConnection(void* parameter);

void setup() {
  // Initialize serial communication for USB debugging
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  // Camera configuration
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  // Set camera settings based on PSRAM availability
  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // Initialize the camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  // Set a lower frame size for faster streaming
  sensor_t* s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_QVGA);  // Lower resolution for faster streaming

  // Initialize WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  // Start the camera server
  startCameraServer();

  // Start the socket client in a separate task to handle TCP connection and data
  xTaskCreate(handleSocketConnection, "Socket Task", 4096, NULL, 1, NULL);

  Serial.print("Camera and Socket Client ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
}

void loop() {
  // Main loop remains empty, as socket handling runs in a separate task
  delay(10000);  // Delay to keep loop idle
}

// Function to handle the socket connection and receive ultrasonic data
void handleSocketConnection(void* parameter) {
  while (true) {
    // Attempt to connect to the transmitter (TCP server)
    if (!socketClient.connected()) {
      Serial.println("Connecting to the socket server...");
      if (socketClient.connect(socket_host, socket_port)) {
        Serial.println("Connected to the socket server.");
      } else {
        Serial.println("Failed to connect to the socket server.");
        delay(5000);  // Wait before retrying
        continue;
      }
    }

    // Read data from the socket when connected
    while (socketClient.connected()) {
      if (socketClient.available()) {
        String data = socketClient.readStringUntil('\n');  // Read the data sent by the transmitter
        Serial.println("Received ultrasonic distance: " + data);  // Print received data to serial
      }
      delay(100);  // Add some delay to avoid overwhelming the loop
    }

    // If disconnected, attempt to reconnect
    if (!socketClient.connected()) {
      Serial.println("Socket disconnected. Reconnecting...");
    }
    delay(1000);  // Wait before attempting to reconnect
  }
}

// Dummy camera server function
void startCameraServer() {
  // This is a placeholder for the actual code to start a camera web server
  Serial.println("Camera server started.");
}
