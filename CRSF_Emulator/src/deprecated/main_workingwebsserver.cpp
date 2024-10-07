// #include <Arduino.h>
// #include <WiFi.h>
// #include <ESPAsyncWebServer.h>

// #define TXD_PIN (GPIO_NUM_17)
// #define RXD_PIN (GPIO_NUM_16) // RX pin (not used in this case)

// uint16_t channels[16];
// uint8_t payload[22];
// uint8_t crsfFrame[26];

// // Wi-Fi credentials
// const char* ssid = "ESP32_Channel_Control";
// const char* password = "12345678";

// AsyncWebServer server(80);

// // Control variables for channels
// float roll = 0.0;     // Channel 1
// float pitch = 0.0;    // Channel 2
// float throttle = 0.0; // Channel 3
// float yaw = 0.0;      // Channel 4
// bool isArmed = false; // Channel 5 (Aux 1)

// // Optional: Control variables for auxiliary channels (Channels 6-16)
// float auxChannels[11]; // Channels 6 to 16

// // HTML content
// const char index_html[] PROGMEM = R"rawliteral(
// 	<!DOCTYPE HTML><html>
// 	<head>
// 	<title>ESP32 Channel Control</title>
// 	<meta name="viewport" content="width=device-width, initial-scale=1">
// 	<style>
// 		html { font-family: Arial; display: inline-block; text-align: center; }
// 		h2 { font-size: 2.0rem; }
// 		p { font-size: 1.5rem; }
// 		.slider { width: 300px; }
// 		.button { padding: 15px 50px; font-size: 24px; margin: 5px; }
// 		.slider-container { margin: 20px 0; }
// 	</style>
// 	</head>
// 	<body>
// 	<h2>ESP32 Channel Control</h2>
	
// 	<div class="slider-container">
// 		<p>Roll: <span id="rollValue">0</span>%</p>
// 		<input type="range" min="-100" max="100" value="0" class="slider" id="rollSlider">
// 	</div>
	
// 	<div class="slider-container">
// 		<p>Pitch: <span id="pitchValue">0</span>%</p>
// 		<input type="range" min="-100" max="100" value="0" class="slider" id="pitchSlider">
// 	</div>
	
// 	<div class="slider-container">
// 		<p>Yaw: <span id="yawValue">0</span>%</p>
// 		<input type="range" min="-100" max="100" value="0" class="slider" id="yawSlider">
// 	</div>
	
// 	<div class="slider-container">
// 		<p>Throttle: <span id="throttleValue">0</span>%</p>
// 		<input type="range" min="0" max="100" value="0" class="slider" id="throttleSlider">
// 	</div>

// 	<p>
// 		<button class="button" onclick="toggleArm()"><span id="armButton">Arm</span></button>
// 		<button class="button" onclick="stopMotors()">Stop Motors</button>
// 	</p>
	
// 	<script>
// 	var rollSlider = document.getElementById("rollSlider");
// 	var rollValue = document.getElementById("rollValue");

// 	var pitchSlider = document.getElementById("pitchSlider");
// 	var pitchValue = document.getElementById("pitchValue");

// 	var yawSlider = document.getElementById("yawSlider");
// 	var yawValue = document.getElementById("yawValue");

// 	var throttleSlider = document.getElementById("throttleSlider");
// 	var throttleValue = document.getElementById("throttleValue");

// 	var armButton = document.getElementById("armButton");
// 	var isArmed = false;

// 	// Update Roll
// 	rollSlider.oninput = function() {
// 	rollValue.innerHTML = this.value;
// 	var xhr = new XMLHttpRequest();
// 	xhr.open("GET", "/update?roll="+this.value, true);
// 	xhr.send();
// 	}

// 	// Update Pitch
// 	pitchSlider.oninput = function() {
// 	pitchValue.innerHTML = this.value;
// 	var xhr = new XMLHttpRequest();
// 	xhr.open("GET", "/update?pitch="+this.value, true);
// 	xhr.send();
// 	}

// 	// Update Yaw
// 	yawSlider.oninput = function() {
// 	yawValue.innerHTML = this.value;
// 	var xhr = new XMLHttpRequest();
// 	xhr.open("GET", "/update?yaw="+this.value, true);
// 	xhr.send();
// 	}

// 	// Update Throttle
// 	throttleSlider.oninput = function() {
// 	throttleValue.innerHTML = this.value;
// 	var xhr = new XMLHttpRequest();
// 	xhr.open("GET", "/update?throttle="+this.value, true);
// 	xhr.send();
// 	}

// 	// Arm/Disarm
// 	function toggleArm() {
// 	isArmed = !isArmed;
// 	armButton.innerHTML = isArmed ? "Disarm" : "Arm";
// 	var xhr = new XMLHttpRequest();
// 	xhr.open("GET", "/update?arm="+(isArmed ? "1" : "0"), true);
// 	xhr.send();
// 	}

// 	// Stop Motors
// 	function stopMotors() {
// 	// Reset sliders to zero
// 	rollSlider.value = 0;
// 	rollValue.innerHTML = 0;

// 	pitchSlider.value = 0;
// 	pitchValue.innerHTML = 0;

// 	yawSlider.value = 0;
// 	yawValue.innerHTML = 0;

// 	throttleSlider.value = 0;
// 	throttleValue.innerHTML = 0;

// 	isArmed = false;
// 	armButton.innerHTML = "Arm";

// 	// Send updated values to the ESP32
// 	var xhr = new XMLHttpRequest();
// 	xhr.open("GET", "/update?stop=1", true);
// 	xhr.send();

// 	// Send zero values for Roll, Pitch, Yaw, and Throttle
// 	var params = "roll=0&pitch=0&yaw=0&throttle=0";
// 	var xhr2 = new XMLHttpRequest();
// 	xhr2.open("GET", "/update?" + params, true);
// 	xhr2.send();
// 	}
// 	</script>
// 	</body>
// 	</html>
// 	)rawliteral";

// void initializeChannels() {
//   for (int i = 0; i < 16; i++) {
//     channels[i] = 992; // Neutral value for all channels
//   }
  
//   // Initialize auxiliary channels to 0%
//   for (int i = 0; i < 11; i++) {
//     auxChannels[i] = 0.0; // Channels 6 to 16
//   }
// }

// void updateChannels() {
//   channels[0] = constrain((int)(992 + (roll * 820)), 172, 1811);      // Roll
//   channels[1] = constrain((int)(992 + (pitch * 820)), 172, 1811);     // Pitch
//   channels[2] = constrain((int)(172 + (throttle * 1639)), 172, 1811); // Throttle
//   channels[3] = constrain((int)(992 + (yaw * 820)), 172, 1811);       // Yaw

//   // Channel 5 for Arm/Disarm (Aux 1)
//   channels[4] = isArmed ? 1811 : 172; // High value for armed, low for disarmed

//   // Update auxiliary channels (Channels 6 to 16)
//   for (int i = 0; i < 11; i++) {
//     channels[5 + i] = constrain((int)(992 + (auxChannels[i] * 820)), 172, 1811);
//   }
// }

// void packChannels() {
//   uint8_t payloadIndex = 0;
//   uint32_t bitBuffer = 0;
//   uint8_t bitsInBuffer = 0;

//   memset(payload, 0, sizeof(payload));

//   for (uint8_t i = 0; i < 16; i++) {
//     bitBuffer |= ((uint32_t)(channels[i] & 0x07FF)) << bitsInBuffer;
//     bitsInBuffer += 11;

//     while (bitsInBuffer >= 8) {
//       payload[payloadIndex++] = bitBuffer & 0xFF;
//       bitBuffer >>= 8;
//       bitsInBuffer -= 8;
//     }
//   }

//   if (payloadIndex < 22) {
//     payload[payloadIndex] = bitBuffer & 0xFF;
//   }
// }

// uint8_t crc8(const uint8_t *data, uint8_t len) {
//   uint8_t crc = 0;
//   while (len--) {
//     crc ^= *data++;
//     for (uint8_t i = 0; i < 8; i++) {
//       if (crc & 0x80)
//         crc = (crc << 1) ^ 0xD5; // Correct polynomial for CRSF
//       else
//         crc <<= 1;
//     }
//   }
//   return crc;
// }

// void buildCrsfFrame() {
//   crsfFrame[0] = 0xC8;  // Destination Address
//   crsfFrame[1] = 24;    // Length
//   crsfFrame[2] = 0x16;  // Type

//   memcpy(&crsfFrame[3], payload, 22); // Copy Payload

//   crsfFrame[25] = crc8(&crsfFrame[2], 23); // Compute CRC8
// }

// void setup() {
//   Serial.begin(115200); // For debugging via Serial Monitor
//   Serial2.begin(420000, SERIAL_8N1, RXD_PIN, TXD_PIN); // Initialize UART2
//   initializeChannels(); // Initialize channel values

//   // Initialize Wi-Fi
//   WiFi.softAP(ssid, password);
//   IPAddress IP = WiFi.softAPIP();
//   Serial.print("AP IP address: ");
//   Serial.println(IP);

//   // Serve the web page
//   server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
//     request->send_P(200, "text/html", index_html);
//   });

//   // Handle slider and button actions
//   server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request){
//     String inputMessage;

//     if (request->hasParam("roll")) {
//       inputMessage = request->getParam("roll")->value();
//       roll = inputMessage.toFloat() / 100.0;
//       Serial.println("Roll set to " + String(roll * 100) + "%");
//     }
//     if (request->hasParam("pitch")) {
//       inputMessage = request->getParam("pitch")->value();
//       pitch = inputMessage.toFloat() / 100.0;
//       Serial.println("Pitch set to " + String(pitch * 100) + "%");
//     }
//     if (request->hasParam("yaw")) {
//       inputMessage = request->getParam("yaw")->value();
//       yaw = inputMessage.toFloat() / 100.0;
//       Serial.println("Yaw set to " + String(yaw * 100) + "%");
//     }
//     if (request->hasParam("throttle")) {
//       inputMessage = request->getParam("throttle")->value();
//       throttle = inputMessage.toFloat() / 100.0;
//       Serial.println("Throttle set to " + String(throttle * 100) + "%");
//     }
//     if (request->hasParam("arm")) {
//       inputMessage = request->getParam("arm")->value();
//       isArmed = inputMessage == "1";
//       Serial.println(isArmed ? "Armed" : "Disarmed");
//     }
//     if (request->hasParam("stop")) {
//       // Stop motors by setting throttle to zero and disarming
//       throttle = 0.0;
//       isArmed = false;

//       // Reset Roll, Pitch, and Yaw to zero
//       roll = 0.0;
//       pitch = 0.0;
//       yaw = 0.0;

//       Serial.println("Motors stopped and controls reset");
//     }
//     // Optional: Handle auxiliary channels
//     /*
//     if (request->hasParam("aux1")) {
//       inputMessage = request->getParam("aux1")->value();
//       auxChannels[0] = inputMessage.toFloat() / 100.0; // Aux 1 is index 0
//       Serial.println("Aux 1 set to " + String(auxChannels[0] * 100) + "%");
//     }
//     */

//     request->send(204); // No content response
//   });

//   server.begin();
// }

// void loop() {
//   updateChannels();
//   packChannels();
//   buildCrsfFrame();

//   Serial2.write(crsfFrame, 26); // Send CRSF frame

//   delay(20); // 50Hz update rate
// }
