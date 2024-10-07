<p align="center">
  <img src="https://static.vecteezy.com/system/resources/thumbnails/049/514/995/small/drone-hovering-in-mid-air-above-a-landscape-during-daylight-cut-out-transparent-png.png" alt="5552-drone">
  <h1>ELEC5552 Electrical Design Project</h1>
 
</p>

## Overview
This repository houses all components of the ELEC5552 Electrical Design project conducted at University of Western Australia. The project includes comprehensive hardware designs, build manuals and software implementations for a comprehensive autonomous drone.

### Key Features:
- **Autonomous Line Following**: Adjusts drone pitch and yaw to maintain alignment with a predefined path based on camera input.
- **Obstacle Detection**: Uses an ultrasonic sensor to detect and avoid obstacles by adjusting throttle or stopping if necessary.
- **Safe Operation**: Monitors communication status and applies safety protocols in case of communication loss.
- **Manual Mode**: Allows manual control when selected, with smooth switching between manual and autonomous modes.


## Hardware Requirements

- **Flight Controller**: Responsible for stabilizing and controlling the drone's flight based on sensor inputs.
- **Bottom-facing Camera**: Used for detecting and following a line or path.
- **Ultrasonic Sensor**: Detects obstacles and assists with altitude management and obstacle avoidance.
- **Motors and ESCs**: To control the drone's flight movements (throttle, pitch, and yaw).
- **Radio or Wi-Fi Module**: For communication between the base station and the drone, ensuring the drone receives commands and maintains safe operation.
- **Battery (800mAh - 1S/2S)**: Powers the drone and onboard electronics.

## Software Requirements

- **Arduino IDE**: Used to program the flight controller and integrate the sensor inputs.
- **Communication Protocol**: Wireless communication between the drone and the controller, such as LoRa, Wi-Fi, or a proprietary radio system.
- **Sensor Drivers**: Libraries or drivers for handling the ultrasonic sensor and camera data.

## Contributors
- Jonathon Sader
- William Hor
- Sep Kimiaei
- Carl Alvares
- Liam Kubach
- Aidan Crummey



