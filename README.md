# GPS-tracker

ESP-IDF firmware project for a GPS logger currently configured for `esp32s3`. The firmware initializes a UART GPS module, waits for a valid fix, and appends coordinates to a CSV file on an SD card at a configurable interval.

## Project Layout

- `main/` - ESP-IDF entrypoint and top-level wiring
- `components/gps_logger/` - GPS UART setup, TinyGPS++ parsing, SD card mounting, CSV logging
- `managed_components/` - dependencies fetched by the ESP-IDF component manager
- `archive/arduino/` - archived Arduino prototypes kept only as project history
- `GPS-tracker.code-workspace` - portable VS Code workspace settings for this repo

## Dependencies

The firmware uses:

- ESP-IDF drivers for UART, SPI and FATFS/SD
- `cinderblocks/esp_tinygpsplusplus` via the ESP Component Registry for NMEA parsing

When ESP-IDF tools are exported, initialize the target and build with:

```bash
idf.py set-target esp32s3
idf.py reconfigure
idf.py build
```

## Default Wiring

The current defaults mirror the archived `gps_light.ino` wiring and keep a standard SPI layout for the SD card:

- GPS RX on GPIO `3`
- GPS TX on GPIO `2`
- GPS baud rate `9600`
- SD CS on GPIO `4`
- SD SCK on GPIO `18`
- SD MISO on GPIO `19`
- SD MOSI on GPIO `23`

These values live in `components/gps_logger/include/gps_logger.hpp`.

## Logging Format

Each day is written to `/sdcard/YYYYMMDD.csv` with the header:

```text
date,time,latitude,longitude,altitude_m,speed_kmph,course_deg
```

Each row is formatted as:

```text
YYYY.MM.DD,HH:MM:SS,55.75580000,37.61730000,156.20,12.40,84.50
```

## Development Notes

- `managed_components/` and `dependencies.lock` are generated and refreshed by the ESP-IDF component manager when dependencies change.
- `build/`, `sdkconfig`, and `.vscode/settings.json` can contain machine-specific or target-specific state and may be regenerated locally.
- On a new machine, let the ESP-IDF VS Code extension select the local ESP-IDF installation instead of committing local tool paths into the repository.
