<p align="center">
  <img src="https://i.imgur.com/MdkiKY9.png" alt="ELEC5552 Drone Image" width="400">
  <h1>ELEC5552 Electrical Design Project</h1>
</p>

---

## Overview

Welcome to the repository for the **ELEC5552 Electrical Design Project** at the University of Western Australia. This project encompasses hardware designs, build guides, and software implementations for developing a fully autonomous drone. The goal is to design and build a robust drone capable of autonomous line following, obstacle detection, and smooth transitions between manual and autonomous control modes.

### Key Features

- **Autonomous Line Following**  
  The drone adjusts its pitch and yaw to maintain alignment with a predefined path using a bottom-facing camera.

- **Obstacle Detection**  
  An ultrasonic sensor detects obstacles, allowing the drone to either stop or adjust its throttle to avoid collisions.

- **Safe Operation**  
  In the event of communication loss, the drone follows safety protocols to ensure secure operation.

- **Manual Mode**  
  The system enables manual control for the operator, with seamless switching between manual and autonomous modes.

---

## Hardware Requirements

- **Flight Controller**  
  Central unit that stabilizes and manages the drone’s flight based on input from sensors.

- **Bottom-facing Camera**  
  Provides vision for line/path detection to guide the drone during autonomous mode.

- **Ultrasonic Sensor**  
  Facilitates obstacle detection and altitude management to prevent collisions.

- **Motors and ESCs (Electronic Speed Controllers)**  
  Essential for precise control over throttle, pitch, and yaw during flight.

- **Radio or Wi-Fi Module**  
  Ensures wireless communication between the drone and the base station for real-time command transmission.

- **Battery (800mAh - 1S/2S)**  
  Powers the drone and onboard electronics.

---

## Software Requirements

- **Arduino IDE**  
  Development environment used to program the flight controller and integrate sensor inputs.

- **Communication Protocol**  
  Implements wireless communication between the drone and controller using protocols like LoRa, Wi-Fi, or proprietary radio systems.

- **Sensor Drivers**  
  Drivers or libraries are used for managing data from the ultrasonic sensor and camera.

---

## Contributors

- Jonathon Sader  
- William Hor  
- Sep Kimiaei  
- Carl Alvares  
- Liam Kubach  
- Aidan Crummey

---
