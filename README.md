# SmartBin

**An IoT-Based Smart Waste Sorting System Using ESP32 and Edge Impulse**

SmartBin is an intelligent waste bin prototype designed to monitor waste capacity and sort waste into two categories: organic and non-organic.

The project uses ESP32-based hardware, sensors, a servo motor, and an Edge Impulse machine learning model to support automated waste classification.

## Project Overview

SmartBin is developed as a smart waste management prototype. It combines embedded systems, IoT components, and machine learning to demonstrate an automated waste sorting concept.

### Main Features

- Waste classification into organic and non-organic categories.
- Waste capacity monitoring using an ultrasonic sensor.
- Waste detection using an infrared sensor.
- Automatic sorting mechanism powered by a servo motor.
- Image-based waste classification using an ESP32-CAM and an Edge Impulse model.

## Hardware Components

| Component | Function |
|---|---|
| ESP32 | Main system controller |
| ESP32-CAM | Image capture and machine learning inference |
| Ultrasonic Sensor | Measures waste bin capacity |
| Infrared Sensor | Detects incoming waste |
| Servo Motor | Controls the sorting plate |

## Machine Learning

The project uses an Edge Impulse model trained to classify waste images into two categories:

- Organic
- Non-Organic

The exported Arduino library is provided in the `lib/` directory.

## Repository Structure

```text
SmartBin/
├── SmartBin.ino
├── README.md
├── LICENSE
├── .gitignore
├── dataset/
├── lib/
├── docs/
│   ├── images/
│   ├── circuit-diagram/
│   └── testing/
└── hardware/
    └── SmartBin3D.obj
```

## Installation and Setup

### Requirements

- Arduino IDE
- ESP32 board package for Arduino IDE
- Required Arduino libraries
- Edge Impulse Arduino library
- ESP32 and ESP32-CAM boards

### Setup Instructions

1. Clone or download this repository.
2. Install Arduino IDE.
3. Install the ESP32 board package.
4. Install the required Arduino libraries.
5. Import the Edge Impulse Arduino library ZIP from the `lib/` directory.
6. Open `SmartBin.ino` using Arduino IDE.
7. Select the appropriate ESP32 board and COM port.
8. Connect the hardware according to the wiring diagram in `docs/circuit-diagram/`.
9. Upload the program to the ESP32.

## Documentation

Additional project documentation is available in the `docs/` directory:

- Circuit diagrams.
- Usage and testing documentation.
- Project images.

The 3D model is available in the `hardware/` directory.

## Circuit Diagram

![SmartBin Wiring Diagram](docs/circuit-diagram/SmartBin%20Wiring%20Diagram_bb.png)

## Dataset

The `dataset/` directory contains image samples used for the waste classification project.

## Project Status

**Prototype V1**

This repository represents the first prototype of SmartBin. Further improvements and hardware integration may be developed in future versions.

## License

This project is licensed under the MIT License. See the `LICENSE` file for details.
