# Tilt_Password_Project
Tilt-based password system using STM32 and accelerometer

## Notes
- 📄 [Download full report](./Weekly_Notes_Project_1.txt)

## Report
- 📄 [Download full report](./Embedded_Systems_C22303056_20_03_2026_Project_1_Design_Doc.pdf)

## Schematic
- 📄 [Download full report](./Schematic.pdf)
## Code

Main source files:

- [main.c](./main.c) – Main password logic  
- [display.c](./display.c) – Display functions  
- [i2c.c](./i2c.c) – Accelerometer communication  
- [spi.c](./spi.c) – Display communication  
- [eeng1030_lib.c](./eeng1030_lib.c) – Utility functions  

## Features
- Tilt-based password input  
- LED feedback system  
- I2C communication with accelerometer  
- SPI display interface  
- UART debugging using serial monitor  
# Tilt Password Project

## Overview
This project implements a tilt-based password system using sensor input. The system detects specific tilt movements and compares them against a predefined sequence to grant access.

## Features
- Tilt-based password input
- Sensor-driven interaction
- Real-time validation of input sequence
- Simple and intuitive user interface

## How It Works
The system uses a tilt sensor (or accelerometer) to detect directional movement. Each tilt corresponds to a specific input. A correct sequence of tilts acts as the password.

## How to Run
1. Clone the repository:
   git clone https://github.com/Ck-coder-boop/Tilt_Password_Project

2. Open the project in the required environment VS code

3. Upload the code to the microcontroller

4. Press the button to initilise the sequence

5. Input the tilt sequence Left, right, forward and back

## Demo Video and code explanation
https://youtu.be/VWH-4uiciJQ

## Author
Conor King
