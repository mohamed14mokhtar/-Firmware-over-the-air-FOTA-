# 📡 Firmware Over-The-Air (FOTA) for STM32F4

---

## 📋 Table of Contents

1. [Project Overview](#-project-overview)
2. [System Components](#system-components)
   - [Hardware Components](#hardware-components)
   - [Software Tools](#software-tools)
   - [Communication Interfaces](#communication-interfaces)
3. [System Workflow](#system-workflow)
4. [Web Server Interface](#web-server-interface)

---

<p align="center">
  <img src="https://github.com/user-attachments/assets/61d0638d-89ac-4667-bea5-48093a8dce58" alt="FOTA System Diagram">
</p>

## 🧾 Project Overview

This project implements a complete **Firmware Over-The-Air (FOTA)** system for the STM32 microcontroller (Cortex-M4). Its primary goal is to enable wireless firmware updates without needing a physical connection to the microcontroller.

The FOTA system consists of three main components:

1. **Custom Bootloader**: A lightweight bootloader programmed on the STM32 that handles receiving firmware over UART and writing it to flash memory.
2. **ESP8266 Wi-Fi Module (ESP-01)**: Serves as a wireless bridge between the local web server and the STM32, receiving firmware updates over Wi-Fi and transmitting them via UART.
3. **Node-RED Web Server**: A user-friendly web interface running on a local machine, allowing users to upload `.bin` firmware files for remote flashing.

This project is ideal for embedded systems used in remote locations or enclosed environments where physical firmware updates are challenging. It provides a reliable and scalable solution for wireless microcontroller programming.

---

## 🔧 System Components

### 🖧 Hardware Components

- **STM32F401RCT6 (ARM Cortex-M4)**: Main microcontroller running the bootloader and the user application.
- **ESP-01 (ESP8266 Wi-Fi Module)**: Provides wireless communication for firmware transfer.
- **LEDs**: Can be used to display firmware update status or debug messages.
- 
<p align="center">
  <img src="[https://github.com/user-attachments/assets/89f2f2f1-b8d0-4ed4-9094-fe42ef050f8e" alt="Node-RED Web Interface](https://github.com/user-attachments/assets/bef1a0cc-0cb5-4807-b833-240c6629fee5)">
</p>
---

### 💻 Software Tools

- **STM32CubeMX**: Used to configure STM32 peripherals and generate initialization code.
- **STM32CubeIDE**: Development and debugging environment for writing both bootloader and application.
- **Keil MDK-ARM**: An alternative development environment for building and debugging firmware on STM32.
- **Arduino IDE**: Used for writing and uploading code to the ESP-01 module.
- **Node-RED**: Provides a simple web-based interface for uploading firmware and managing FOTA.

---

### 📡 Communication Interfaces

- **UART**: Serial communication channel between STM32 and ESP-01 for transferring the binary firmware.
- **HTTP (POST)**: Used by Node-RED to send the `.bin` file wirelessly to the ESP-01.
- **Wi-Fi (802.11)**: Enables the ESP-01 to connect to the local network and communicate with the web interface.

---

## 🧭 System Workflow

The Firmware Over-The-Air (FOTA) system follows these steps for a successful wireless firmware update:

1. **Bootloader Initialization**:
   - The STM32F401RCT6 starts up and enters the bootloader.
   - The bootloader checks for a firmware update request from the ESP-01 via UART.

2. **Node-RED Web Interface**:
   - The user accesses a local web page built with Node-RED.
   - A `.bin` firmware file is selected and uploaded through the interface.
   - Node-RED sends the firmware to the ESP-01 module using an HTTP POST request.

3. **ESP-01 Wireless Transfer**:
   - The ESP-01 receives the firmware file and starts sending it to the STM32 over UART in chunks.
   - The STM32 bootloader writes the received data to flash memory after verifying it.

4. **Completion & Validation**:
   - Once the transfer is complete, the bootloader verifies that the firmware is valid.
   - If valid, the system jumps to the main application code.
   - Optionally, status is displayed via an LED or LCD module.

5. **Future Updates**:
   - When a new firmware is available, the process can be repeated at any time via the same Node-RED interface.

---

<p align="center">
  <img src="https://github.com/user-attachments/assets/386c0bfd-76f5-4fb2-b458-431644dfabf4" alt="Firmware Update Process">
</p>

## 🌐 Web Server Interface

The Node-RED web interface facilitates seamless firmware uploads and system monitoring. Below is a screenshot of the interface:

<p align="center">
  <img src="https://github.com/user-attachments/assets/89f2f2f1-b8d0-4ed4-9094-fe42ef050f8e" alt="Node-RED Web Interface">
</p>

*Figure: Node-RED Web Interface for Firmware Uploads*

---

