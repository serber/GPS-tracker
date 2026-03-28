# GPS-tracker

ESP-IDF firmware project for an `esp32` GPS logger. The firmware initializes a UART GPS module, waits for a valid fix, and appends coordinates to a CSV file on an SD card at a configurable interval.

## Project Layout

- `main/` - ESP-IDF entrypoint and top-level wiring
- `components/gps_logger/` - GPS UART setup, TinyGPS++ parsing, SD card mounting, CSV logging
- `archive/arduino/` - archived Arduino prototypes kept only as project history

## Dependencies

The firmware uses:

- ESP-IDF drivers for UART, SPI and FATFS/SD
- `cinderblocks/esp_tinygpsplusplus` via the ESP Component Registry for NMEA parsing

When ESP-IDF tools are exported, fetch and build dependencies with:

```bash
idf.py reconfigure
idf.py build
```

## Default Wiring

The current defaults mirror the archived `gps_light.ino` wiring and keep a standard ESP32 VSPI layout for the SD card:

- GPS RX on ESP32 GPIO `3`
- GPS TX on ESP32 GPIO `2`
- GPS baud rate `9600`
- SD CS on GPIO `4`
- SD SCK on GPIO `18`
- SD MISO on GPIO `19`
- SD MOSI on GPIO `23`

These values live in `components/gps_logger/include/gps_logger.hpp`.

## Logging Format

Each day is written to `/sdcard/YYYYMMDD.csv` with the header:

```text
date,time,latitude,longitude
```

Each row is formatted as:

```text
YYYY.MM.DD,HH:MM:SS,55.75580000,37.61730000
```

## Notes

- `GPIO 3` is also the default UART0 RX pin on many ESP32 boards. If you use the USB serial console and a GPS module on the same line, move the GPS RX pin in `gps_logger.hpp`.
- `idf.py` was not available in the current shell during this pass, so the firmware still needs a real `idf.py build` check in an ESP-IDF environment.
