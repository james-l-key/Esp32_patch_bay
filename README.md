# Esp32_Patch_Bay

An 8-pedal mono guitar patch bay controlled by an ESP32-S3, allowing dynamic reconfiguration of guitar pedal signal chains via a user interface with an OLED display and push buttons. The project uses analog multiplexers (CD4051B, ADG419BN) and op-amps (TL072) to route audio signals, controlled through shift registers (74HC595) interfaced with the ESP32-S3. The schematic is designed in KiCad 9.0, and the firmware is developed using the ESP-IDF framework.

## Table of Contents
- [Features](#features)
- [Hardware](#hardware)
- [Software](#software)
- [Installation](#installation)
- [Usage](#usage)
- [Project Structure](#project-structure)
- [Scaling to 8 Pedals](#scaling-to-8-pedals)
- [Contributing](#contributing)
- [License](#license)
- [Acknowledgements](#acknowledgements)

## Features
- **Dynamic Signal Routing**: Reconfigure the signal chain for up to 8 mono guitar pedals using a 9x8 matrix.
- **User Interface**: SSD1306/SH1106 OLED display for configuration feedback, with three push buttons (Program, Pedal 1, Pedal 2) and a Program Mode LED.
- **Low-Latency Audio**: CD4051B and ADG419BN analog multiplexers and TL072 op-amps ensure clean, buffered audio signal routing.
- **ESP32-S3 Control**: High-performance microcontroller for real-time control, programmed via ESP-IDF.
- **Extensible Design**: Easily scalable to support more pedals by adding multiplexers and shift registers.
- **Open Source**: Full schematic (`circuit.kicad_sch`), PCB layout, and firmware provided.

## Hardware
The patch bay is built around the following components:
- **Microcontroller**: ESP32-S3 (e.g., ESP32-S3-WROOM-1 module).
- **Display**: SSD1306/SH1106 128x64 OLED (I2C interface).
- **Audio Routing**:
  - CD4051B 8-channel analog multiplexers and ADG419BN SPDT analog switches for signal path selection.
  - 3x TL072 dual op-amps for input/output buffering.
  - 6x 1/4” mono jacks (Guitar In/Out, Pedal 1 In/Out, Pedal 2 In/Out).
- **Control**:
  - 2x 74HC595 8-bit shift registers to control multiplexer select lines.
  - 3x push buttons (Program, Pedal 1, Pedal 2) with pull-up resistors.
  - 1x LED for Program Mode indication.
- **Power**:
  - +3.3V for ESP32-S3, OLED, and 74HC595.
  - ±9V for analog components (CD4051B, ADG419BN, and TL072) to handle audio signals.
- **Schematic**: Designed in KiCad 9.0 (`circuit.kicad_sch`).

### Power Requirements
- **Input Power**: The system is designed for an external **12V DC power supply** connected via a barrel jack.
- **Isolated Dual Rails (±15V)**: A TMA-1215D DC/DC converter (U3) takes the 12V input and generates isolated **±15V** rails, which are used by the analog audio circuitry.
- **Regulated +5V**: A linear regulator (e.g., LM7805) provides +5V, primarily for digital components if needed, and as an input to the 3.3V regulator.
- **Regulated -5V**: A linear regulator (e.g., LM7905) provides -5V.
- **Regulated +3.3V**: A linear regulator (e.g., AMS1117-3.3) provides +3.3V for the ESP32-S3 and OLED display.
- Ensure proper decoupling capacitors (e.g., 100nF, 10µF) near each IC.

## Software
The firmware is written in C using the **ESP-IDF** framework (v5.1 or later) for the ESP32-S3. Key components include:
- **Matrix Control**: `matrix.c` manages the 9x8 signal routing matrix, interfacing with 74HC595 shift registers to set analog switch states.
- **OLED Driver**: Displays the current signal chain and configuration status.
- **Button Handling**: Debounced inputs for Program, Pedal 1, and Pedal 2 buttons to navigate and set configurations.
- **LED Control**: Toggles the Program Mode LED based on user interaction.

### Dependencies
- **ESP-IDF**: Install ESP-IDF v5.1 or later (see [Espressif Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/)).
- **KiCad 9.0**: For viewing/editing the schematic (`circuit.kicad_sch`).
- **I2C Library**: For SSD1306/SH1106 OLED communication (included in ESP-IDF).
- **GPIO Drivers**: Standard ESP-IDF GPIO functions for buttons, LED, and shift registers.

## Installation
### Prerequisites
- **Hardware**:
  - ESP32-S3 development board or custom PCB with the components listed above.
  - 1/4” mono jacks, buttons, LED, resistors (220Ω for LED, 4.7kΩ for pull-ups), and capacitors.
  - ±9V power supply or charge pump circuit.
- **Software**:
  - [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/get-started/index.html) installed and configured.
  - [KiCad 9.0](https://www.kicad.org/download/) for schematic/PCB design.
  - A C compiler (e.g., `xtensa-esp32s3-elf-gcc`) included with ESP-IDF.
  - Python 3 for ESP-IDF build scripts.

### Steps
1. **Clone the Repository**:
   ```bash
   git clone [https://github.com/phi/Esp32_patch_bay.git](https://github.com/phi/Esp32_patch_bay.git)
   cd Esp32_patch_bay
   
2. **Set Up ESP-IDF**:
        Follow the ESP-IDF Getting Started Guide.
        Ensure the idf.py tool is accessible in your terminal.

3. **Build the Firmware**:
    ```bash
    idf.py set-target esp32s3
    idf.py build
   
5. **Flash the Firmware**:

    Connect the ESP32-S3 to your computer via USB.
    Flash the firmware:
    ```bash

        idf.py -p /dev/ttyUSB0 flash

  Replace /dev/ttyUSB0 with your serial port (e.g., COM3 on Windows).

  6. **Open the Schematic**:
        Launch KiCad 9.0.
        Open circuit.kicad_sch from the repository root.
        Verify connections and assign footprints for PCB layout.

   7. **Assemble the Hardware**:
        Solder components onto a PCB or prototype board based on the schematic.
        Connect 1/4” jacks to guitar, pedals, and amplifier.
        Power the circuit with +3.3V and ±9V supplies.

## Usage

  **Power On**:
        Connect the patch bay to power (+3.3V, ±9V).
        The OLED displays the current signal chain (e.g., Guitar → Pedal 1 → Pedal 2 → Amp).

  **Program Mode**:
        Press the Program button (SW1) to enter Program Mode (LED1 lights up).
        Use Pedal 1 (SW2) and Pedal 2 (SW3) buttons to select the desired signal path.
        Press Program again to save and exit.

  **Signal Routing**:
        The ESP32-S3 updates the 74HC595 shift registers, which set the analog switches to route the audio signal.
        TL072 op-amps buffer the input and output to maintain signal integrity.

  **Monitor**:
        Use the OLED to confirm the signal chain.
        Check the serial output (via idf.py monitor) for debugging.

***Example Signal Chains***

  * Bypass: Guitar → Amp.
  * Single Pedal: Guitar → Pedal 1 → Amp.
  * Dual Pedal: Guitar → Pedal 1 → Pedal 7 → Amp or Guitar → Pedal 2 → Pedal 1 → Amp.

## Project Structure

```md
Esp32_patch_bay/
├── circuit.kicad_sch        # KiCad 9.0 schematic for 3x3 matrix (scalable to 9x8)
├── main/
│   ├── main.c               # Entry point for ESP-IDF firmware
│   ├── matrix.c             # Signal routing matrix control
│   ├── matrix.h             # Header for matrix functions
│   ├── oled.c               # OLED display driver
│   ├── oled.h               # Header for OLED functions
├── CMakeLists.txt           # ESP-IDF project configuration
├── README.md                # This file
├── LICENSE                  # License file (e.g., MIT)
└── docs/
    ├── HARDWARE.md          # Detailed hardware setup
    ├── SOFTWARE.md          # Firmware development guide
    ├── TROUBLESHOOTING.md   # Common issues and solutions
```
## Scaling to 8 Pedals

**The current schematic implements a 9x8 matrix (Guitar In, Amp Out, and 8 pedals In/Out).**


## Contributing

**Contributions are welcome! To contribute**:

  Fork the repository.
  Create a branch (git checkout -b feature/your-feature).
  Commit changes (git commit -m "Add your feature").
  Push to the branch (git push origin feature/your-feature).
  Open a Pull Request.

**Please include**:

  Detailed descriptions of changes.
  Updates to documentation if necessary.
  Tests or verification steps for hardware/firmware changes.

## Issues

  Report bugs or suggest features via the Issues tab.
  Include reproduction steps and relevant logs (e.g., idf.py monitor output).

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

## Acknowledgements

  Espressif Systems for the ESP-IDF framework and ESP32-S3.
  KiCad for open-source EDA tools.
  Community: Inspiration from guitar pedalboard designs and open-source hardware projects.
