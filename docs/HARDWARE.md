# Hardware Setup

## Bill of Materials (BOM)
| Component          | Quantity | Notes                                      |
|--------------------|----------|--------------------------------------------|
| ESP32-S3 Module    | 1        | ESP32-S3-DevKitC recommended               |
| SSD1306 OLED       | 1        | 128x64, I2C interface                      |
| CD4051B Multiplexer| Multiple | Analog multiplexer for signal routing      |
| ADG419BN Analog Switch| Multiple | SPDT analog switch for signal routing      |
| TL072 Op-Amp       | 3        | Dual op-amp for buffering                  |
| 74HC595 Shift Reg. | 2        | 8-bit shift register                       |
| 1/4” Mono Jack     | 18        | For guitar, pedals, and amp                |
| Push Button        | 10        | Tactile switches                           |
| LED                | 1        | 3mm, any color                             |
| Resistor (220Ω)    | 1        | For LED                                    |
| Resistor (4.7kΩ)   | 10        | For button pull-ups                        |
| Capacitor (100nF)  | 10       | Decoupling for ICs                         |
| TMA-1215D DC/DC Converter | 1        | Generates isolated ±15V from 12V input [cite: 536] |
| LM7805_TO220       | 1        | Positive 5V linear regulator [cite: 536]   |
| LM7905_TO220       | 1        | Negative 5V linear regulator [cite: 536]   |
| AMS1117CD-3.3      | 1        | 3.3V linear regulator [cite: 536]          |
| Polyfuse           | 1        | Resettable fuse for input protection [cite: 536] |
| 1N4007 Diode       | 1        | Rectifier diode for input protection [cite: 536] |
| Barrel Jack        | 1        | For 12V DC power input [cite: 536]         |

## Power Supply
- **Input Power**: The system is designed for an external **12V DC power supply** connected via a barrel jack[cite: 536].
- **Isolated Dual Rails (±15V)**: A TMA-1215D DC/DC converter (U3) takes the 12V input and generates isolated **±15V** rails, which are used by the analog audio circuitry[cite: 536].
- **Regulated +5V**: An LM7805 linear regulator (U2) steps the +15V rail down to a stable **+5V** output, used for various digital components and as input for the 3.3V regulator[cite: 536].
- **Regulated -5V**: An LM7905 linear regulator (U4) converts the -15V rail to a stable **-5V** output[cite: 536].
- **Regulated +3.3V**: An AMS1117-3.3 linear regulator (U1) steps the +5V rail down to a stable **+3.3V** output for the ESP32-S3 and OLED display[cite: 536].
- Ensure proper decoupling capacitors (e.g., 100nF, 10µF) near each IC.

## PCB Design
- Open `circuit.kicad_sch` in KiCad 9.0.
- Assign footprints (e.g., SOIC-16 for CD4051B, DIP-8 for TL072, TO-252-3 for AMS1117-3.3, TO-220-3 for LM7805/LM7905, SIP-7 for TMA-1215D).
- Route PCB with separate analog/digital ground planes for noise reduction.

## Assembly Tips
- Solder ICs first, then passives, then connectors.
- Test power rails before powering on.
- Verify jack connections with a multimeter.
