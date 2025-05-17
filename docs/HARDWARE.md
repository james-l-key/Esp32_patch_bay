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
