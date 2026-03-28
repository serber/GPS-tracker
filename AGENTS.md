# Repository Guidelines

## Project Structure & Module Organization
This repository is an ESP-IDF firmware project currently configured for `esp32s3`. The root [CMakeLists.txt](CMakeLists.txt) defines the project, [main/main.cpp](main/main.cpp) contains the `app_main()` entry point, and [main/CMakeLists.txt](main/CMakeLists.txt) wires the main application component. Reusable firmware logic lives in [components/gps_logger](components/gps_logger), third-party registry dependencies are resolved into `managed_components/`, and historical Arduino prototypes live under [archive/arduino/](archive/arduino). Editor and workspace setup lives in `.devcontainer/`, `.vscode/`, and [GPS-tracker.code-workspace](GPS-tracker.code-workspace).

## Build, Test, and Development Commands
Run commands from the repository root after exporting ESP-IDF tools.

- `idf.py set-target esp32s3` initializes the current target once per workspace.
- `idf.py build` configures CMake and builds the firmware into `build/`.
- `idf.py flash monitor` flashes the device and opens the serial monitor.
- `idf.py fullclean` removes generated build artifacts when configuration gets stale.
- `idf.py reconfigure` refreshes CMake and managed component resolution after dependency or target changes.

If you use VS Code on a new machine, let the ESP-IDF extension select the local ESP-IDF installation rather than committing machine-specific paths into the repository. The included devcontainer is still the safest way to get a matching toolchain.

## Coding Style & Naming Conventions
Use C/C++ for firmware sources, 4-space indentation, and braces on their own lines, matching [main/main.cpp](main/main.cpp). Prefer `snake_case` for functions and variables, and keep filenames lowercase (`gps_parser.c`, `uart_reader.c`). Keep hardware-specific constants near the module that owns them. Run `clangd`-backed formatting or your editor’s C formatter before submitting; avoid unrelated whitespace churn.

## Testing Guidelines
There is no automated test suite yet. Treat `idf.py build` as the minimum validation step for every change. When behavior touches serial I/O, GPS parsing, SD card access, or board wiring, include a short manual test note in the PR describing hardware used, flashed image, and observed serial output or generated CSV files. Add future tests under `main/` or `components/` alongside the code they verify.

## Commit & Pull Request Guidelines
Current history uses short, imperative commit subjects such as `Initial commit` and `Add files via upload`; keep that style, but make messages more specific, for example `Add UART GPS parser`. PRs should include a concise summary, linked issue if applicable, build status, and screenshots or serial logs for hardware-visible changes. Keep PRs focused on one firmware change at a time.
