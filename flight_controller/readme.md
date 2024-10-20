

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
