# Repository Guidelines

## Project Structure & Module Organization
This repository is an ESP-IDF firmware project for `esp32`. The root [CMakeLists.txt](/Users/albert/Documents/sources/GPS-tracker/CMakeLists.txt) defines the project, and [main/main.c](/Users/albert/Documents/sources/GPS-tracker/main/main.c) contains the `app_main()` entry point built by ESP-IDF. Use [main/CMakeLists.txt](/Users/albert/Documents/sources/GPS-tracker/main/CMakeLists.txt) to register additional source files as the firmware grows. The top-level [gps.ino](/Users/albert/Documents/sources/GPS-tracker/gps.ino) and [gps_light.ino](/Users/albert/Documents/sources/GPS-tracker/gps_light.ino) are reference sketches and are not part of the current CMake build. Container and editor setup lives in `.devcontainer/` and `.vscode/`.

## Build, Test, and Development Commands
Run commands from the repository root after exporting ESP-IDF tools.

- `idf.py set-target esp32` initializes the target once per workspace.
- `idf.py build` configures CMake and builds the firmware into `build/`.
- `idf.py flash monitor` flashes the device and opens the serial monitor.
- `idf.py fullclean` removes generated build artifacts when configuration gets stale.

If you use VS Code, the included devcontainer is based on `espressif/idf` and is the safest way to get a matching toolchain.

## Coding Style & Naming Conventions
Use C for firmware sources, 4-space indentation, and braces on their own lines, matching [main/main.c](/Users/albert/Documents/sources/GPS-tracker/main/main.c). Prefer `snake_case` for functions and variables, and keep filenames lowercase (`gps_parser.c`, `uart_reader.c`). Keep hardware-specific constants near the module that owns them. Run `clangd`-backed formatting or your editor’s C formatter before submitting; avoid unrelated whitespace churn.

## Testing Guidelines
There is no automated test suite yet. Treat `idf.py build` as the minimum validation step for every change. When behavior touches serial I/O, GPS parsing, or board wiring, include a short manual test note in the PR describing hardware used, flashed image, and observed output. Add future tests under `main/` or `components/` alongside the code they verify.

## Commit & Pull Request Guidelines
Current history uses short, imperative commit subjects such as `Initial commit` and `Add files via upload`; keep that style, but make messages more specific, for example `Add UART GPS parser`. PRs should include a concise summary, linked issue if applicable, build status, and screenshots or serial logs for hardware-visible changes. Keep PRs focused on one firmware change at a time.
