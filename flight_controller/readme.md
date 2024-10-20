<p align="center">
  <img src="https://i.imgur.com/MdkiKY9.png" alt="Team01 Drone" width="400">
  <h1>Team01 Drone Control Firmware</h1>
</p>

---

## Overview

Welcome to the **Team01 Drone Control Firmware** repository. This project is an Arduino-based firmware designed to manage and control an autonomous drone. Leveraging the power of the ESP32 microcontroller, this firmware facilitates seamless communication between the drone's Flight Controller (FC) and a ground station via Wi-Fi. It supports real-time telemetry, control commands, and video streaming, ensuring robust and responsive drone operations.

---

## Key Features

- **Wireless Communication**: Connects to a ground station using Wi-Fi, enabling remote control and monitoring.
- **CRSF Protocol Support**: Implements the Crossfire (CRSF) protocol for efficient and reliable communication with the Flight Controller.
- **Real-Time Telemetry**: Transmits essential telemetry data such as battery status, RSSI, accelerometer readings, barometer data, and motor outputs to the ground station.
- **WebSocket Integration**: Facilitates bidirectional communication between the drone and ground station for sending commands and receiving telemetry.
- **Web Server**: Hosts a web interface for monitoring and controlling the drone.
- **Control Commands**: Supports commands like Roll, Pitch, Yaw, Throttle, Arm/Disarm, and Emergency Stop.
- **Simulation Mode**: Allows simulation of drone states for testing purposes without actual hardware.

---

## Hardware Requirements

To successfully deploy and operate the Team01 Drone Control Firmware, ensure you have the following hardware components:

- **ESP32 Development Board**: Serves as the main controller handling Wi-Fi communication, WebSocket server, and CRSF protocol.
- **Flight Controller (FC)**: Manages the drone's flight dynamics and receives commands via CRSF.
- **UART Connection**:
  - **TXD_PIN (GPIO 17)**: ESP32 TX Pin connected to FC RX.
  - **RXD_PIN (GPIO 16)**: ESP32 RX Pin connected to FC TX.
- **Power Supply**: Adequate power source to run the ESP32 and connected peripherals.
- **Sensors and Actuators**:
  - **Ultrasonic Sensors**: For obstacle detection.
  - **Motors and ESCs (Electronic Speed Controllers)**: To control drone movement.
- **Additional Components**: Depending on specific project needs, such as GPS modules, cameras, etc.

---

## Software Requirements

- **Arduino IDE**: The primary development environment for writing, compiling, and uploading the firmware to the ESP32.
- **ESPAsyncWebServer Library**: Facilitates asynchronous web server functionalities on the ESP32.
- **AsyncTCP Library**: Required dependency for ESPAsyncWebServer.
- **LittleFS Library**: Provides a lightweight file system for storing web assets like HTML, CSS, and JavaScript files.
- **ArduinoJson Library**: Enables efficient JSON parsing and serialization for telemetry data.
- **CRSF Protocol Support**: Custom implementation within the firmware for communication with the Flight Controller.

### Installing Required Libraries

1. **Open Arduino IDE**.
2. **Navigate to** `Sketch` > `Include Library` > `Manage Libraries`.
3. **Search and Install** the following libraries:
   - `ESPAsyncWebServer`
   - `AsyncTCP`
   - `LittleFS`
   - `ArduinoJson`

*Alternatively, you can install these libraries via the Arduino Library Manager by searching for their names.*

---

## Understanding the CRSF Protocol

### What is CRSF?

**CRSF (Crossfire)** is a proprietary communication protocol developed by Team BlackSheep for long-range, low-latency communication between a transmitter and a receiver, typically used in RC (Radio Control) applications such as drones and other remote-controlled vehicles. CRSF is renowned for its high reliability, low latency, and efficient bandwidth utilization.

### Why CRSF?

- **Low Latency**: Ensures rapid response times, crucial for real-time control in dynamic environments.
- **High Reliability**: Maintains stable connections even in challenging conditions, minimizing packet loss.
- **Efficient Bandwidth**: Optimizes data transmission, allowing for more telemetry data without overloading the communication channel.
- **Extended Range**: Provides robust communication over longer distances compared to traditional RC protocols.

### CRSF in Team01 Firmware

In the Team01 Drone Control Firmware, CRSF is utilized to facilitate communication between the ESP32 microcontroller and the Flight Controller (FC). Here's how CRSF is integrated:

1. **UART Communication**: The ESP32 communicates with the FC via UART using designated TX and RX pins.
2. **CRSF Frames**: Data is encapsulated in CRSF frames, which include a synchronization byte, length, type, payload, and CRC for error checking.
3. **Telemetry Data**: The firmware parses incoming telemetry packets from the FC, extracting vital information such as battery status, RSSI, accelerometer readings, barometer data, and motor outputs.
4. **Control Commands**: The firmware sends control commands (e.g., Roll, Pitch, Yaw, Throttle, Arm/Disarm) to the FC by constructing and transmitting appropriate CRSF frames.
5. **Error Handling**: Implements CRC checks to ensure data integrity and handles reconnection logic in case of communication failures.

### CRSF Frame Structure

A typical CRSF frame consists of the following components:

1. **Sync Byte**: `0xC8` indicating the start of a CRSF frame.
2. **Length**: Specifies the total length of the frame.
3. **Type**: Indicates the type of data being transmitted (e.g., Binding, Channel Data, Telemetry).
4. **Payload**: Contains the actual data, such as channel values or telemetry information.
5. **CRC**: Cyclic Redundancy Check byte for error detection.

### CRC8 Calculation

The firmware includes a `crc8` function that calculates the CRC for a given data buffer. This ensures that the data transmitted and received is free from errors. The CRC8 uses a polynomial (`0xD5`) specific to the CRSF protocol.

```cpp
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
