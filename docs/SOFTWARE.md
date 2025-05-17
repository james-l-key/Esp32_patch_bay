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
