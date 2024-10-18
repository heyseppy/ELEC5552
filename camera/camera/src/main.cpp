#include "esp_camera.h"
#include <WiFi.h>

// Camera pin configuration (adjust these for your ESP32-CAM module)
#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      4
#define SIOD_GPIO_NUM     18
#define SIOC_GPIO_NUM     23

#define Y9_GPIO_NUM       36
#define Y8_GPIO_NUM       37
#define Y7_GPIO_NUM       38
#define Y6_GPIO_NUM       39
#define Y5_GPIO_NUM       35
#define Y4_GPIO_NUM       32
#define Y3_GPIO_NUM       33
#define Y2_GPIO_NUM       34
#define VSYNC_GPIO_NUM     5
#define HREF_GPIO_NUM     27
#define PCLK_GPIO_NUM     25

// Wi-Fi credentials (ESP32-CAM will act as an access point)
const char* ssid = "ESP32_CAM_AP";
const char* password = "12345678";

// Wi-Fi server on port 80 for HTTP and WebSocket handshake
WiFiServer server(80);

// Global variables
float line_angle = 0.0;

// Function to initialize the camera
void startCamera() {
  camera_config_t config;
  config.ledc_channel    = LEDC_CHANNEL_0;
  config.ledc_timer      = LEDC_TIMER_0;
  config.pin_d0          = Y2_GPIO_NUM;
  config.pin_d1          = Y3_GPIO_NUM;
  config.pin_d2          = Y4_GPIO_NUM;
  config.pin_d3          = Y5_GPIO_NUM;
  config.pin_d4          = Y6_GPIO_NUM;
  config.pin_d5          = Y7_GPIO_NUM;
  config.pin_d6          = Y8_GPIO_NUM;
  config.pin_d7          = Y9_GPIO_NUM;
  config.pin_xclk        = XCLK_GPIO_NUM;
  config.pin_pclk        = PCLK_GPIO_NUM;
  config.pin_vsync       = VSYNC_GPIO_NUM;
  config.pin_href        = HREF_GPIO_NUM;
  config.pin_sscb_sda    = SIOD_GPIO_NUM;
  config.pin_sscb_scl    = SIOC_GPIO_NUM;
  config.pin_pwdn        = PWDN_GPIO_NUM;
  config.pin_reset       = RESET_GPIO_NUM;
  config.xclk_freq_hz    = 20000000;  // 20MHz
  config.pixel_format    = PIXFORMAT_JPEG;

  // Set frame size and quality
  config.frame_size      = FRAMESIZE_QQVGA;  // 160x120 resolution
  config.jpeg_quality    = 12;               // Adjust quality (lower number is higher quality)
  config.fb_count        = 1;                // Single buffer

  // Initialize the camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    ESP.restart();
  }
}

// Function to calculate the angle of the black line
void calculateLineAngle(camera_fb_t *fb) {
  // Convert the JPEG image to grayscale
  size_t out_len = fb->width * fb->height;
  uint8_t *gray_buf = (uint8_t *)malloc(out_len);
  if (!gray_buf) {
    Serial.println("Memory allocation failed");
    line_angle = 0.0;
    return;
  }

  // Decode JPEG image to RGB565
  if (!fmt2rgb565(fb->buf, fb->len, fb->format, gray_buf)) {
    Serial.println("Failed to convert frame to RGB565");
    line_angle = 0.0;
    free(gray_buf);
    return;
  }

  // Convert RGB565 to grayscale
  for (size_t i = 0; i < out_len / 2; i++) {
    uint16_t pixel = ((uint16_t *)gray_buf)[i];
    uint8_t r = (pixel >> 11) & 0x1F;
    uint8_t g = (pixel >> 5) & 0x3F;
    uint8_t b = pixel & 0x1F;
    uint8_t gray = (r * 76 + g * 150 + b * 29) >> 8;
    gray_buf[i] = gray;
  }

  // Simple line detection logic
  int sum_x = 0;
  int count = 0;
  for (int y = 0; y < fb->height; y++) {
    for (int x = 0; x < fb->width; x++) {
      uint8_t pixel = gray_buf[y * fb->width + x];
      if (pixel < 50) {  // Threshold for black pixels
        sum_x += x;
        count++;
      }
    }
  }

  if (count > 0) {
    int avg_x = sum_x / count;
    // Calculate angle based on avg_x position
    line_angle = ((float)avg_x / fb->width) * 180.0 - 90.0;  // Angle from -90 to +90 degrees
  } else {
    line_angle = 0.0;
  }

  free(gray_buf);
}

// Function to handle WebSocket data framing
void sendWebSocketFrame(WiFiClient client, const uint8_t *payload, size_t length) {
  uint8_t header[10];
  size_t headerSize = 0;

  header[0] = 0x82;  // Binary frame opcode
  if (length <= 125) {
    header[1] = length;
    headerSize = 2;
  } else if (length <= 65535) {
    header[1] = 126;
    header[2] = (length >> 8) & 0xFF;
    header[3] = length & 0xFF;
    headerSize = 4;
  } else {
    header[1] = 127;
    header[2] = (length >> 56) & 0xFF;
    header[3] = (length >> 48) & 0xFF;
    header[4] = (length >> 40) & 0xFF;
    header[5] = (length >> 32) & 0xFF;
    header[6] = (length >> 24) & 0xFF;
    header[7] = (length >> 16) & 0xFF;
    header[8] = (length >> 8) & 0xFF;
    header[9] = length & 0xFF;
    headerSize = 10;
  }

  client.write(header, headerSize);
  client.write(payload, length);
}

void setup() {
  Serial.begin(115200);
  Serial.println();

  // Initialize the camera
  startCamera();

  // Start the Wi-Fi Access Point
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("Access Point IP address: ");
  Serial.println(IP);

  // Start the server
  server.begin();
  Serial.println("Server started");
}

void loop() {
  WiFiClient client = server.available();
  if (client) {
    Serial.println("New client connected");
    String request = client.readStringUntil('\r');
    Serial.println(request);
    client.flush();

    if (request.indexOf("GET / ") >= 0) {
      // Serve the main page
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/html");
      client.println("Connection: close");
      client.println();
      client.println("<!DOCTYPE html>");
      client.println("<html>");
      client.println("<head>");
      client.println("<title>ESP32-CAM Line Angle Detection</title>");
      client.println("</head>");
      client.println("<body>");
      client.println("<h1>ESP32-CAM Line Angle Detection</h1>");
      client.println("<canvas id=\"canvas\" width=\"160\" height=\"120\"></canvas>");
      client.println("<p>Line Angle: <span id=\"angle\">0</span> degrees</p>");
      client.println("<script>");
      client.println("var ws = new WebSocket('ws://' + location.host + '/ws');");
      client.println("ws.binaryType = 'arraybuffer';");
      client.println("ws.onmessage = function(event) {");
      client.println("  var arrayBuffer = event.data;");
      client.println("  var bytes = new Uint8Array(arrayBuffer);");
      client.println("  var blob = new Blob([bytes], {type: 'image/jpeg'});");
      client.println("  var img = new Image();");
      client.println("  img.onload = function() {");
      client.println("    var canvas = document.getElementById('canvas');");
      client.println("    var ctx = canvas.getContext('2d');");
      client.println("    ctx.drawImage(img, 0, 0, canvas.width, canvas.height);");
      client.println("  };");
      client.println("  img.src = URL.createObjectURL(blob);");
      client.println("};");
      client.println("setInterval(function() {");
      client.println("  fetch('/angle').then(function(response) {");
      client.println("    return response.text();");
      client.println("  }).then(function(text) {");
      client.println("    document.getElementById('angle').innerText = text;");
      client.println("  });");
      client.println("}, 500);");
      client.println("</script>");
      client.println("</body>");
      client.println("</html>");
    } else if (request.indexOf("GET /ws") >= 0) {
      // Handle WebSocket handshake
      String key;
      while (client.available()) {
        String line = client.readStringUntil('\n');
        if (line.indexOf("Sec-WebSocket-Key:") >= 0) {
          key = line.substring(line.indexOf(':') + 2, line.length() - 1);
        }
        if (line == "\r") {
          break;
        }
      }

      // Compute Sec-WebSocket-Accept
      key.trim();
      key += "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
      uint8_t sha1Hash[20];
      sha1(key.c_str(), key.length(), sha1Hash);
      char acceptKey[29] = {0};
      base64_encode(sha1Hash, 20, acceptKey);

      // Send handshake response
      client.println("HTTP/1.1 101 Switching Protocols");
      client.println("Upgrade: websocket");
      client.println("Connection: Upgrade");
      client.print("Sec-WebSocket-Accept: ");
      client.println(acceptKey);
      client.println();
      Serial.println("WebSocket handshake completed");

      // Send video frames over WebSocket
      while (client.connected()) {
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb) {
          Serial.println("Camera capture failed");
          break;
        }

        // Calculate line angle
        calculateLineAngle(fb);

        // Send frame over WebSocket
        sendWebSocketFrame(client, fb->buf, fb->len);

        esp_camera_fb_return(fb);

        // Small delay to prevent flooding
        delay(50);
      }
      Serial.println("WebSocket client disconnected");
    } else if (request.indexOf("GET /angle") >= 0) {
      // Serve the line angle
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/plain");
      client.println("Connection: close");
      client.println();
      client.printf("%.2f", line_angle);
    } else {
      // Not found
      client.println("HTTP/1.1 404 Not Found");
      client.println("Connection: close");
      client.println();
    }

    client.stop();
    Serial.println("Client disconnected");
  }
}

// SHA-1 and Base64 functions
#include "mbedtls/sha1.h"
void sha1(const char* input, size_t len, uint8_t* output) {
  mbedtls_sha1_context ctx;
  mbedtls_sha1_init(&ctx);
  mbedtls_sha1_starts_ret(&ctx);
  mbedtls_sha1_update_ret(&ctx, (const unsigned char*)input, len);
  mbedtls_sha1_finish_ret(&ctx, output);
  mbedtls_sha1_free(&ctx);
}

const char* base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
void base64_encode(uint8_t* input, size_t len, char* output) {
  int i = 0, j = 0;
  while (len--) {
    int val = (input[i++] << 16) & 0xFFFFFF;
    if (len--) val |= (input[i++] << 8) & 0xFFFFFF;
    if (len--) val |= (input[i++]) & 0xFFFFFF;

    output[j++] = base64_chars[(val >> 18) & 0x3F];
    output[j++] = base64_chars[(val >> 12) & 0x3F];
    output[j++] = base64_chars[(val >> 6) & 0x3F];
    output[j++] = base64_chars[val & 0x3F];
  }
  int mod = i % 3;
  if (mod == 1) {
    output[--j] = '=';
    output[--j] = '=';
  } else if (mod == 2) {
    output[--j] = '=';
  }
  output[j] = '\0';
}
