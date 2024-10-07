// // Only changed throttle value at 30%
// #include <Arduino.h>

// #define TXD_PIN (GPIO_NUM_17)
// #define RXD_PIN (GPIO_NUM_16) // RX pin (not used in this case)

// uint16_t channels[16];
// uint8_t payload[22];
// uint8_t crsfFrame[26];

// float throttle = 0.0; // Initialize throttle to 0%

// void initializeChannels()
// {
//     for (int i = 0; i < 16; i++)
//     {
//         channels[i] = 992; // Neutral value for all channels
//     }
// }

// void updateChannels(float roll, float pitch, float throttle, float yaw)
// {
//     channels[0] = constrain((int)(992 + (roll * 820)), 172, 1811);  // Roll
//     channels[1] = constrain((int)(992 + (pitch * 820)), 172, 1811); // Pitch
//     channels[2] = constrain((int)(172 + (throttle * 1639)), 172, 1811); // Throttle
//     // channels[2] = constrain((int)(885 + (throttle * (2115 - 885))), 885, 2115);

//     channels[3] = constrain((int)(992 + (yaw * 820)), 172, 1811); // Yaw
// }

// void packChannels()
// {
//     uint8_t bitIndex = 0;
//     memset(payload, 0, 22);
//     for (uint8_t i = 0; i < 16; i++)
//     {
//         uint16_t channelValue = channels[i] & 0x07FF; // Mask to 11 bits
//         for (uint8_t bit = 0; bit < 11; bit++)
//         {
//             if (channelValue & (1 << bit))
//             {
//                 payload[bitIndex / 8] |= (1 << (bitIndex % 8));
//             }
//             bitIndex++;
//         }
//     }
// }

// uint8_t crc8(const uint8_t *data, uint8_t len)
// {
//     uint8_t crc = 0;
//     while (len--)
//     {
//         uint8_t extract = *data++;
//         for (uint8_t i = 8; i; i--)
//         {
//             uint8_t sum = (crc ^ extract) & 0x01;
//             crc >>= 1;
//             if (sum)
//             {
//                 crc ^= 0x8C; // Polynomial 0x31 reversed
//             }
//             extract >>= 1;
//         }
//     }
//     return crc;
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
//         }
//         else if (c == 's')
//         {
//             throttle -= 0.05; // Decrease throttle by 5%
//             if (throttle < 0.0)
//                 throttle = 0.0; // Floor at 0%
//             Serial.println("Throttle decreased to " + String(throttle * 100) + "%");
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

//     // Use zero values for roll, pitch, and yaw in this example
//     float roll = 0.0;
//     float pitch = 0.0;
//     float yaw = 0.0;

//     updateChannels(roll, pitch, throttle, yaw);

//     Serial.println(channels[2]); // Print raw throttle channel value

//     packChannels();

//     for (int i = 0; i < 22; i++) {
//         Serial.print(payload[i], HEX);
//         Serial.print(" ");
//     }
//     Serial.println();

//     buildCrsfFrame();

//     // for (int i = 0; i < 26; i++) {
//     // Serial.print(crsfFrame[i], HEX);
//     // Serial.print(" ");
//     // }
//     // Serial.println();

//     Serial2.write(crsfFrame, 26); // Send CRSF frame

//     delay(20); // 50Hz update rate
// }
