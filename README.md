# Esp32_Patch_Bay

An 8-pedal mono guitar patch bay controlled by an ESP32-S3, allowing dynamic reconfiguration of guitar pedal signal chains via a user interface with an OLED display and push buttons. The project uses analog multiplexers (DG408) and op-amps (TL072) to route audio signals, controlled through shift registers (74HC595) interfaced with the ESP32-S3. The schematic is designed in KiCad 9.0, and the firmware is developed using the ESP-IDF framework.

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
- **Dynamic Signal Routing**: Reconfigure the signal chain for up to 8 mono guitar pedals using a 3x3 matrix (scalable to 9x9).
- **User Interface**: SSD1306/SH1106 OLED display for configuration feedback, with three push buttons (Program, Pedal 1, Pedal 2) and a Program Mode LED.
- **Low-Latency Audio**: DG408 multiplexers and TL072 op-amps ensure clean, buffered audio signal routing.
- **ESP32-S3 Control**: High-performance microcontroller for real-time control, programmed via ESP-IDF.
- **Extensible Design**: Easily scalable to support more pedals by adding multiplexers and shift registers.
- **Open Source**: Full schematic (`circuit.kicad_sch`), PCB layout, and firmware provided.

## Hardware
The patch bay is built around the following components:
- **Microcontroller**: ESP32-S3 (e.g., ESP32-S3-WROOM-1 module).
- **Display**: SSD1306/SH1106 128x64 OLED (I2C interface).
- **Audio Routing**:
  - 4x DG408 8:1 analog multiplexers for signal path selection.
  - 3x TL072 dual op-amps for input/output buffering.
  - 6x 1/4” mono jacks (Guitar In/Out, Pedal 1 In/Out, Pedal 2 In/Out).
- **Control**:
  - 2x 74HC595 8-bit shift registers to control multiplexer select lines.
  - 3x push buttons (Program, Pedal 1, Pedal 2) with pull-up resistors.
  - 1x LED for Program Mode indication.
- **Power**:
  - +3.3V for ESP32-S3, OLED, and 74HC595.
  - ±9V for DG408 and TL072 to handle audio signals.
- **Schematic**: Designed in KiCad 9.0 (`circuit.kicad_sch`).

### Power Requirements
- **+3.3V**: Supplied via a regulator (e.g., AMS1117-3.3) from a 5V USB or external source.
- **±9V**: Generated using a dual-rail power supply or charge pump (e.g., ICL7660 for -9V).
- Ensure proper decoupling capacitors (e.g., 100nF, 10µF) near each IC.

## Software
The firmware is written in C using the **ESP-IDF** framework (v5.1 or later) for the ESP32-S3. Key components include:
- **Matrix Control**: `matrix.c` manages the 3x3 signal routing matrix, interfacing with 74HC595 shift registers to set DG408 multiplexer states.
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
   git clone https://github.com/phi/Esp32_patch_bay.git
   cd Esp32_patch_bay
   ```

2. **Set Up ESP-IDF**:
   - Follow the [ESP-IDF Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/get-started/index.html).
   - Ensure the `idf.py` tool is accessible in your terminal.

3. **Build the Firmware**:
   ```bash
   idf.py set-target esp32s3
   idf.py build
   ```

4. **Flash the Firmware**:
   - Connect the ESP32-S3 to your computer via USB.
   - Flash the firmware:
     ```bash
     idf.py -p /dev/ttyUSB0 flash
     ```
   - Replace `/dev/ttyUSB0` with your serial port (e.g., `COM3` on Windows).

5. **Open the Schematic**:
   - Launch KiCad 9.0.
   - Open `circuit.kicad_sch` from the repository root.
   - Verify connections and assign footprints for PCB layout.

6. **Assemble the Hardware**:
   - Solder components onto a PCB or prototype board based on the schematic.
   - Connect 1/4” jacks to guitar, pedals, and amplifier.
   - Power the circuit with +3.3V and ±9V supplies.

## Usage
1. **Power On**:
   - Connect the patch bay to power (+3.3V, ±9V).
   - The OLED displays the current signal chain (e.g., Guitar → Pedal 1 → Pedal 2 → Amp).

2. **Program Mode**:
   - Press the **Program** button (SW1) to enter Program Mode (LED1 lights up).
   - Use **Pedal 1** (SW2) and **Pedal 2** (SW3) buttons to select the desired signal path.
   - Press **Program** again to save and exit.

3. **Signal Routing**:
   - The ESP32-S3 updates the 74HC595 shift registers, which set the DG408 multiplexers to route the audio signal.
   - TL072 op-amps buffer the input and output to maintain signal integrity.

4. **Monitor**:
   - Use the OLED to confirm the signal chain.
   - Check the serial output (via `idf.py monitor`) for debugging.

### Example Signal Chains
- **Bypass**: Guitar → Amp.
- **Single Pedal**: Guitar → Pedal 1 → Amp.
- **Dual Pedal**: Guitar → Pedal 1 → Pedal 2 → Amp or Guitar → Pedal 2 → Pedal 1 → Amp.

## Project Structure
```
Esp32_patch_bay/
├── circuit.kicad_sch        # KiCad 9.0 schematic for 3x3 matrix
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
The current schematic implements a **3x3 matrix** (Guitar, Pedal 1, Pedal 2, Amp). To scale to 8 pedals (9x9 matrix):
1. **Add Jacks**:
   - Include 6 additional 1/4” mono jacks (Pedal 3–8 In/Out) in the schematic.
   - Update `circuit.kicad_sch` to connect new jacks to additional DG408 multiplexers.
2. **Add Multiplexers**:
   - Add 5 more DG408 ICs (total 9) to handle 9 input/output nodes.
   - Connect select lines (A0–A2, EN) to additional 74HC595 outputs or ESP32 GPIOs.
3. **Expand Shift Registers**:
   - Add 1–2 more 74HC595 ICs to provide enough control lines for all DG408s.
   - Daisy-chain them with U2/U3 (Q7S to DS).
4. **Update Firmware**:
   - Modify `matrix.c` to support a 9x9 matrix (see below).
   - Update GPIO assignments if additional shift registers are used.
5. **Power Supply**:
   - Ensure the ±9V supply can handle increased current for additional DG408s and TL072s.
   - Add decoupling capacitors for new ICs.

### Updated matrix.c
Below is an updated `matrix.c` for the 3x3 matrix, compatible with the provided schematic. It can be extended for a 9x9 matrix by adjusting the `MATRIX_SIZE` define and GPIO pin assignments.

**Save as** `main/matrix.c`:
```c
#include "matrix.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define MATRIX_SIZE 3
#define SR_DS   GPIO_NUM_12   // 74HC595 DS (data)
#define SR_SHCP GPIO_NUM_13   // 74HC595 SHCP (shift clock)
#define SR_STCP GPIO_NUM_18   // 74HC595 STCP (latch)
#define SR_OE   GPIO_NUM_19   // 74HC595 OE (output enable, active low)

// DG408 control structure: {A0, A1, A2, EN} for each multiplexer
static const uint8_t dg408_pins[4][4] = {
    {0, 1, 2, 3},  // U4: Row 1
    {4, 5, 6, 7},  // U5: Row 2
    {8, 9, 10, 11}, // U6: Row 3
    {12, 13, 14, 15} // U7: Column
};

// Current routing state
static uint8_t routing[MATRIX_SIZE][MATRIX_SIZE];

void matrix_init(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << SR_DS) | (1ULL << SR_SHCP) | (1ULL << SR_STCP) | (1ULL << SR_OE),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    gpio_set_level(SR_OE, 0); // Enable output
    gpio_set_level(SR_STCP, 0);
    gpio_set_level(SR_SHCP, 0);
    gpio_set_level(SR_DS, 0);

    // Initialize routing matrix to bypass (Guitar -> Amp)
    for (int i = 0; i < MATRIX_SIZE; i++) {
        for (int j = 0; j < MATRIX_SIZE; j++) {
            routing[i][j] = (i == 0 && j == 2) ? 1 : 0; // Guitar (0) to Amp (2)
        }
    }
    matrix_update();
}

void matrix_set_route(uint8_t in, uint8_t out) {
    if (in >= MATRIX_SIZE || out >= MATRIX_SIZE) return;
    for (int i = 0; i < MATRIX_SIZE; i++) {
        routing[in][i] = (i == out) ? 1 : 0;
    }
    matrix_update();
}

void matrix_update(void) {
    uint16_t sr_data = 0;
    for (int row = 0; row < MATRIX_SIZE; row++) {
        for (int col = 0; col < MATRIX_SIZE; col++) {
            if (routing[row][col]) {
                int mux_idx = row;
                int sel = col; // Select line (0-7 for DG408)
                sr_data |= (sel & 0x01) << dg408_pins[mux_idx][0];
                sr_data |= ((sel >> 1) & 0x01) << dg408_pins[mux_idx][1];
                sr_data |= ((sel >> 2) & 0x01) << dg408_pins[mux_idx][2];
                sr_data |= 1 << dg408_pins[mux_idx][3]; // Enable
            }
        }
    }

    // Shift out 16 bits to two 74HC595s
    for (int i = 15; i >= 0; i--) {
        gpio_set_level(SR_DS, (sr_data >> i) & 0x01);
        gpio_set_level(SR_SHCP, 1);
        vTaskDelay(1 / portTICK_PERIOD_MS);
        gpio_set_level(SR_SHCP, 0);
    }
    gpio_set_level(SR_STCP, 1);
    vTaskDelay(1 / portTICK_PERIOD_MS);
    gpio_set_level(SR_STCP, 0);
}
```

**Header file** (`main/matrix.h`):
```c
#ifndef MATRIX_H
#define MATRIX_H

void matrix_init(void);
void matrix_set_route(uint8_t in, uint8_t out);
void matrix_update(void);

#endif
```

**Main file** (`main/main.c`):
```c
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "matrix.h"

void app_main(void) {
    matrix_init();
    // Example: Route Guitar (0) to Pedal 1 (1)
    matrix_set_route(0, 1);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    // Route Pedal 1 (1) to Amp (2)
    matrix_set_route(1, 2);

    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
```

### Scaling matrix.c
For a 9x9 matrix:
- Update `MATRIX_SIZE` to 9.
- Expand `dg408_pins` to include 9 multiplexers (36 control lines, requiring ~5 74HC595s).
- Adjust `routing` array and GPIO assignments for additional shift registers.
- Example:
  ```c
  #define MATRIX_SIZE 9
  static const uint8_t dg408_pins[9][4] = {
      {0, 1, 2, 3},   // U4
      {4, 5, 6, 7},   // U5
      {8, 9, 10, 11}, // U6
      {12, 13, 14, 15}, // U7
      {16, 17, 18, 19}, // U8
      {20, 21, 22, 23}, // U9
      {24, 25, 26, 27}, // U10
      {28, 29, 30, 31}, // U11
      {32, 33, 34, 35}  // U12
  };
  ```

## Contributing
Contributions are welcome! To contribute:
1. Fork the repository.
2. Create a branch (`git checkout -b feature/your-feature`).
3. Commit changes (`git commit -m "Add your feature"`).
4. Push to the branch (`git push origin feature/your-feature`).
5. Open a Pull Request.

Please include:
- Detailed descriptions of changes.
- Updates to documentation if necessary.
- Tests or verification steps for hardware/firmware changes.

### Issues
- Report bugs or suggest features via the [Issues](https://github.com/phi/Esp32_patch_bay/issues) tab.
- Include reproduction steps and relevant logs (e.g., `idf.py monitor` output).

## License
This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

## Acknowledgements
- **Espressif Systems** for the ESP-IDF framework and ESP32-S3.
- **KiCad** for open-source EDA tools.
- **Community**: Inspiration from guitar pedalboard designs and open-source hardware projects.

---

### Additional Documentation Files
To make your project more professional, consider adding these files in a `docs/` folder:

1. **HARDWARE.md**:
   ```markdown
   # Hardware Setup

   ## Bill of Materials (BOM)
   | Component          | Quantity | Notes                              |
   |--------------------|----------|------------------------------------|
   | ESP32-S3 Module    | 1        | ESP32-S3-WROOM-1 recommended      |
   | SSD1306 OLED       | 1        | 128x64, I2C interface             |
   | DG408 Multiplexer  | 4        | 8:1 analog multiplexer            |
   | TL072 Op-Amp       | 3        | Dual op-amp for buffering         |
   | 74HC595 Shift Reg. | 2        | 8-bit shift register              |
   | 1/4” Mono Jack     | 6        | For guitar, pedals, and amp       |
   | Push Button        | 3        | Tactile switches                  |
   | LED                | 1        | 3mm, any color                    |
   | Resistor (220Ω)    | 1        | For LED                           |
   | Resistor (4.7kΩ)   | 2        | For button pull-ups               |
   | Capacitor (100nF)  | 10       | Decoupling for ICs                |

   ## Power Supply
   - **+3.3V**: Use a linear regulator (e.g., AMS1117-3.3) from 5V USB.
   - **±9V**: Use a dual-rail supply or charge pump (e.g., ICL7660 for -9V).
   - Ensure sufficient current (e.g., 500mA for ±9V).

   ## PCB Design
   - Open `circuit.kicad_sch` in KiCad 9.0.
   - Assign footprints (e.g., SOIC-16 for DG408, DIP-8 for TL072).
   - Route PCB with separate analog/digital ground planes for noise reduction.

   ## Assembly Tips
   - Solder ICs first, then passives, then connectors.
   - Test power rails before powering on.
   - Verify jack connections with a multimeter.
   ```

2. **SOFTWARE.md**:
   ```markdown
   # Software Development

   ## ESP-IDF Setup
   - Install ESP-IDF v5.1: [Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/get-started/index.html).
   - Set up the toolchain: `xtensa-esp32s3-elf-gcc`.

   ## Firmware Structure
   - `main.c`: Entry point, initializes matrix and runs main loop.
   - `matrix.c/h`: Controls signal routing via 74HC595 and DG408.
   - `oled.c/h`: Drives the SSD1306/SH1106 OLED display.

   ## Adding Features
   - **Button Debouncing**: Implement in `main.c` using FreeRTOS timers.
   - **OLED Menus**: Extend `oled.c` for interactive configuration.
   - **EEPROM Storage**: Save signal chains to ESP32’s flash.

   ## Debugging
   - Use `idf.py monitor` for serial output.
   - Enable verbose logging in `matrix.c` for shift register states.
   ```

3. **TROUBLESHOOTING.md**:
   ```markdown
   # Troubleshooting

   ## Firmware Issues
   - **ESP32 Not Booting**:
     - Check USB connection and serial port.
     - Ensure BOOT pin is not held low.
     - Run `idf.py flash monitor` to diagnose.
   - **Matrix Not Updating**:
     - Verify GPIO assignments in `matrix.c`.
     - Check 74HC595 connections (DS, SHCP, STCP, OE).

   ## Hardware Issues
   - **No Audio**:
     - Test jack connections with a multimeter.
     - Verify TL072 power (±9V) and DG408 select lines.
   - **OLED Not Displaying**:
     - Confirm I2C connections (SDA, SCL).
     - Check +3.3V supply to OLED.
   - **Buttons Not Responding**:
     - Ensure 4.7kΩ pull-up resistors are connected.
     - Test GPIO inputs with a multimeter.

   ## Common Errors
   - **KiCad Schematic Fails to Load**:
     - Ensure KiCad 9.0 is used (version 20250114).
     - Check for syntax errors in `circuit.kicad_sch`.
   - **Shift Register Misbehavior**:
     - Slow down shift clock (add delays in `matrix_update`).
     - Verify VCC/GND connections to 74HC595.
   ```

### Notes
- **Placeholders**: The `circuit.kicad_sch` includes placeholder footprints and datasheets. Assign actual footprints (e.g., SOIC-16 for DG408, 3mm LED) in KiCad’s PCB editor and link datasheets for each component.
- **Testing**: Before scaling to 8 pedals, prototype the 3x3 matrix to verify audio quality and control logic.
- **GitHub Actions**: Consider adding a `.github/workflows/build.yml` to automate ESP-IDF builds:
  ```yaml
  name: Build ESP-IDF
  on: [push, pull_request]
  jobs:
    build:
      runs-on: ubuntu-latest
      steps:
      - uses: actions/checkout@v3
      - uses: espressif/esp-idf-ci-action@v1
        with:
          esp_idf_version: v5.1
          target: esp32s3
  ```
- **Visuals**: Add a schematic screenshot or PCB render to the README (upload to `docs/images/` and link with `![Schematic](docs/images/schematic.png)`).
- **License**: Create a `LICENSE` file with the MIT License:
  ```
  MIT License

  Copyright (c) 2025 Phi

  Permission is hereby granted, free of charge, to any person obtaining a copy...
  ```

### Next Steps
1. Save the `README.md` and additional files in your repository.
2. Test the `circuit.kicad_sch` in KiCad 9.0 to ensure it loads without errors.
3. Flash the provided firmware and verify the 3x3 matrix functionality.
4. Expand the schematic and `matrix.c` for 8 pedals as needed.
5. Push to GitHub and update the repository description to match the README.

If you need specific sections expanded (e.g., detailed PCB layout guide, OLED driver code), or help with GitHub setup (e.g., adding Actions, creating a release), let me know!
