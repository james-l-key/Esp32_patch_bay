# Troubleshooting

## Firmware Issues
- **ESP32 Not Booting**:
  - Check USB connection and serial port.
  - Ensure BOOT pin is not held low.
  - Run `idf.py flash monitor` to diagnose.
- **Matrix Not Updating**:
  - Verify GPIO assignments in `matrix.c`.
  - Check 74HC595 and analog switch connections.

## Hardware Issues
- **No Audio**:
  - Test jack connections with a multimeter.
  - Verify TL072 power (±9V) and analog switch select lines.
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
  - Verify VCC/GND connections to 74HC595 and analog switches.
