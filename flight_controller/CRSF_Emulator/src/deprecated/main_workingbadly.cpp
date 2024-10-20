// #include <Arduino.h>

// #define TXD_PIN (GPIO_NUM_17)
// #define RXD_PIN (GPIO_NUM_16) // RX pin (not used in this case)

// #define ROLL_PITCH_YAW_CENTER 992
// #define ROLL_PITCH_YAW_SCALE 820
// #define THROTTLE_LOW 548
// #define THROTTLE_HIGH 1510
// #define CHANNEL_LOW 172
// #define CHANNEL_HIGH 1811

// uint16_t channels[16];
// uint8_t payload[22];
// uint8_t crsfFrame[26];

// float throttle = 0.5; // Initialize throttle to 0%
// float roll = 0;
// float pitch = 0;
// float yaw = 0;

// void initializeChannels()
// {
//     for (int i = 0; i < 16; i++)
//     {
//         channels[i] = 992; // Neutral value for all channels
//     }
// }

// void armDisarm(bool arm){
//     if(arm){
//         channels[4] = 1811;
//     } else {
//         channels[4] = 172;
//     }
// }

// void updateChannels(float roll, float pitch, float throttle, float yaw)
// {

//     channels[0] = constrain((int)(ROLL_PITCH_YAW_CENTER + (roll * ROLL_PITCH_YAW_SCALE)), CHANNEL_LOW, CHANNEL_HIGH);  // Roll
//     channels[1] = constrain((int)(ROLL_PITCH_YAW_CENTER + (pitch * ROLL_PITCH_YAW_SCALE)), CHANNEL_LOW, CHANNEL_HIGH); // Pitch
//     // channels[2] = constrain((int)(ROLL_PITCH_YAW_CENTER + (throttle * ROLL_PITCH_YAW_SCALE)), CHANNEL_LOW, CHANNEL_HIGH); // Pitch
//     channels[2] = constrain((int)(THROTTLE_LOW + (throttle * (THROTTLE_HIGH - THROTTLE_LOW))), THROTTLE_LOW, THROTTLE_HIGH); // Throttle
//     channels[3] = constrain((int)(ROLL_PITCH_YAW_CENTER + (yaw * ROLL_PITCH_YAW_SCALE)), CHANNEL_LOW, CHANNEL_HIGH); // Yaw
// }

// void packChannels() {
//   uint8_t payloadIndex = 0;
//   uint16_t buffer = 0;
//   uint8_t bitsInBuffer = 0;

//   memset(payload, 0, 22);

//   for (uint8_t i = 0; i < 16; i++) {
//     buffer |= (channels[i] & 0x07FF) << bitsInBuffer;
//     bitsInBuffer += 11;
//     while (bitsInBuffer >= 8) {
//       payload[payloadIndex++] = buffer & 0xFF;
//       buffer >>= 8;
//       bitsInBuffer -= 8;
//     }
//   }
//   if (payloadIndex < 22) {
//     payload[payloadIndex] = buffer & 0xFF;
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


// void buildCrsfFrame()
// {
//     crsfFrame[0] = 0xC8; // Destination Address
//     crsfFrame[1] = 24;   // Length
//     crsfFrame[2] = 0x16; // Type

//     memcpy(&crsfFrame[3], payload, 22); // Copy Payload

//     crsfFrame[25] = crc8(&crsfFrame[2], 23); // Compute CRC8
// }

// void setup()
// {
//     Serial.begin(115200);                                // For debugging via Serial Monitor
//     Serial2.begin(420000, SERIAL_8N1, RXD_PIN, TXD_PIN); // Initialize UART2
//     initializeChannels();                                // Initialize channel values

//     Serial.println("Type 'w' to increase throttle, 's' to decrease throttle.");
// }

// void loop()
// {
//     // Read inputs from Serial Monitor to adjust throttle
//     while (Serial.available() > 0)
//     {
//         char c = Serial.read();
//         if (c == 'w')
//         {
//             throttle += 0.05; // Increase throttle by 5%
//             if (throttle > 1.0)
//                 throttle = 1.0; // Cap at 100%
//             Serial.println("Throttle increased to " + String(throttle * 100) + "%");
//             Serial.println(channels[2]); // Print raw throttle channel value
//         }
//         else if (c == 's')
//         {
//             throttle -= 0.05; // Decrease throttle by 5%
//             if (throttle < 0.0)
//                 throttle = 0.0; // Floor at 0%
//             Serial.println("Throttle decreased to " + String(throttle * 100) + "%");
//             Serial.println(channels[2]); // Print raw throttle channel value
//         }
//         else if (c == 'a')
//         {
//             armDisarm(true);
//             Serial.println("Arming");
//         }
//         else if (c = 'd')
//         {
//             armDisarm(false);
//             Serial.println("Disarming");
//         }
//         else if (c == '\n' || c == '\r')
//         {
//             // Ignore newline and carriage return characters
//         }
//         else
//         {
//             Serial.println("Unknown input: " + String(c));
//         }
//     }

//     updateChannels(roll, pitch, throttle, yaw);

//     packChannels();

//     buildCrsfFrame();

//     Serial2.write(crsfFrame, 26); // Send CRSF frame

//     delay(20); // 50Hz update rate
// }
