# Vox Desktop Messenger

C++23 + Qt6 desktop client scaffold for Vox Messenger.

## Build Targets

- `vox_core` - core static library (crypto, network, storage, services, UI widgets)
- `vox-desktop` - desktop executable
- `vox_unit_tests` - gtest unit suites
- `vox_integration_tests` - fake-server integration suites
- `vox_vector_tests` - protocol/vector suites
- `vox_qt_tests` - Qt model/widget suites
- `vox_contract_tests` (optional) - real backend contract tests (`VOX_ENABLE_CONTRACT_TESTS=ON`)

## Prerequisites

- CMake 3.25+
- C++23 compiler
- Qt6 (Core, Gui, Widgets, Network, WebSockets, Sql, Concurrent, Multimedia, Svg, Test)
- Git

`libsodium` and `googletest` are fetched via CMake `FetchContent`.

## Configure and Build

```bash
cmake -S . -B build -G Ninja
cmake --build build --target vox-desktop
cmake --build build --target vox_unit_tests vox_integration_tests vox_vector_tests vox_qt_tests
```

## Run

```bash
./build/src/vox-desktop --help
./build/src/vox-desktop
```

## Test

```bash
ctest --test-dir build --output-on-failure
```

CTest labels used in this repo:

- `unit`
- `integration`
- `vectors`
- `qt`
- `contract`

## Environment Variables

- `VOX_BASE_URL` - base server URL (default: `http://127.0.0.1:8080`)
- `VOX_VAULT_PASSWORD` - local vault unlock password (default: `vox-dev-password`)
- `VOX_CONTRACT_BASE_URL` - contract-test server base URL
