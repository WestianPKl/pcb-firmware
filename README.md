# PCB Firmware

Multi-platform embedded firmware for a distributed IoT monitoring system. The system combines an STM32F412RGT6 as the primary controller with optional ESP32 or Raspberry Pi Pico modules for wireless connectivity, sensor integration, and cloud data publishing via MQTT.

---

## Table of Contents

- [Architecture Overview](#architecture-overview)
- [Hardware Targets](#hardware-targets)
- [Supported Peripherals](#supported-peripherals)
- [Project Structure](#project-structure)
- [Build & Flash](#build--flash)
- [Configuration](#configuration)
- [Communication Protocol](#communication-protocol)
- [Data Logging](#data-logging)
- [Production Workflow](#production-workflow)

---

## Architecture Overview

```
┌─────────────────────────────────┐
│        STM32F412RGT6           │  ← Primary controller (C / CMSIS)
│  Sensors · Flash · RTC · CAN   │
│  FRAM config · DMA · ADC · PWM │
└──────────────┬──────────────────┘
               │ UART (115200 baud)
       ┌───────┴───────┐
       │               │
  ┌────▼─────┐   ┌─────▼──────┐
  │  ESP32   │   │  RPi Pico  │  ← MicroPython, optional/alternative
  │  WiFi    │   │  WiFi      │
  │  MQTT    │   │  MQTT      │
  └──────────┘   └────────────┘
       │               │
       └───────┬───────┘
               ▼
          MQTT Broker / Cloud
```

The STM32 handles all time-critical tasks: sensor polling, flash logging, RTC management, CAN bus, PWM, ADC, and peripheral control. The wireless module (ESP32 or Pico) reads configuration from STM32 over UART on startup, then handles WiFi, NTP, and MQTT independently.

---

## Hardware Targets

### STM32F412RGT6 — Primary Controller

| Property | Value |
|---|---|
| Core | ARM Cortex-M4 @ 84 MHz |
| Flash | 1 MB |
| RAM | 256 KB |
| Framework | CMSIS (bare-metal C) |
| Build tool | PlatformIO |
| Debug/Upload | ST-Link |
| Device address | `0xB2` |
| Boot offset | `0x08008000` (32 KB bootloader reserved) |

The firmware is split into two independent PlatformIO projects:

- **Bootloader** — validates application interrupt vectors, then jumps to `0x08008000`
- **Application** — full firmware including all drivers and peripheral logic

### ESP32 — WiFi / MQTT Module

- MicroPython firmware
- Communicates with STM32 via UART (GPIO 20/21, 115200 baud)
- Reads device configuration from STM32 FRAM on startup
- Publishes sensor data to MQTT broker
- NTP time synchronisation
- Debounced interrupt handling for STM32 status changes

### Raspberry Pi Pico — Alternative Sensor / Connectivity Module

- MicroPython firmware
- Standalone sensor data collection (BME280, SHT30, INA226)
- I2C LCD driver with timer-based refresh
- LD2450 60 GHz radar parsing for motion/presence detection
- Relay and buzzer output support
- ADC readings
- WiFi + MQTT (same protocol as ESP32)

---

## Supported Peripherals

### Sensors

| Peripheral | Interface | Function |
|---|---|---|
| BME280 | I2C | Temperature, humidity, pressure |
| SHT40 | I2C | Temperature, humidity (STM32 / Pico) |
| SHT30 | I2C | Temperature, humidity (Pico) |
| INA226 | I2C | Voltage, current, power monitoring |
| LD2450 | UART | 60 GHz radar — up to 3 targets (x/y, speed, distance) |

### Memory & Storage

| Peripheral | Interface | Function |
|---|---|---|
| FM24CL16B | I2C | 16 KB FRAM — device configuration |
| MX25L25673G | SPI | 256 Mb NOR Flash — circular data log |

### Time & Display

| Peripheral | Interface | Function |
|---|---|---|
| MCP7940N | I2C | External RTC with 1 Hz square-wave output |
| I2C LCD | I2C | 2×16 character display |

### I/O

- **8 digital inputs** — buttons, GPIO status
- **8 digital outputs** — LEDs, relays, GPIO control
- **7 PWM channels** — Timer1, Timer2, Timer3, Timer4, Timer8
- **4 ADC channels**
- **CAN bus** — ISO 11898 controller

---

## Project Structure

```
pcb-firmware/
├── stm32/
│   ├── STM32F412RGT6/
│   │   ├── application/
│   │   │   ├── src/            # 26 .c drivers and application logic
│   │   │   ├── include/        # 26 .h headers
│   │   │   ├── linker/         # app.ld (base 0x08008000)
│   │   │   ├── lib/
│   │   │   └── platformio.ini
│   │   └── bootloader/
│   │       ├── src/            # main, systick, uart
│   │       ├── include/
│   │       ├── linker/         # bootloader.ld
│   │       └── platformio.ini
│   └── production_files/
│       ├── production_release.py   # Generates device info page binary
│       ├── uart.py
│       └── info_page.bin
├── esp32/
│   └── micropython/
│       ├── main.py             # Entry point
│       ├── program.py          # Main loop, MQTT, STM32 comms
│       ├── config.py           # Configuration dataclass
│       ├── wireless.py         # WiFi client
│       ├── mqtt_simple.py      # Lightweight MQTT client
│       ├── stm32_uart.py       # UART frame protocol
│       ├── nvs_system.py       # Secrets storage (NVS)
│       └── fast_check.py       # Startup health checks
└── pico/
    └── micropython/
        ├── main.py             # Entry point
        ├── program.py          # Main loop, MQTT, sensors
        ├── config.py
        ├── wireless.py
        ├── i2c_lcd.py          # LCD driver
        ├── lcd_api.py          # LCD abstraction layer
        ├── radar.py            # LD2450 frame parser
        ├── sht30.py / sht40.py # Temperature/humidity sensors
        ├── bme280_i2c.py
        ├── rtc_clock.py        # RTC with timezone support
        ├── mqtt_simple.py
        └── nvs_system.py
```

---

## Build & Flash

### Prerequisites

- [PlatformIO](https://platformio.org/) (CLI or VS Code extension)
- ST-Link debugger
- [MicroPython](https://micropython.org/) toolchain for ESP32 / Pico

### STM32 — Bootloader

```bash
cd stm32/STM32F412RGT6/bootloader
pio build -e genericSTM32F412RG
pio run -e genericSTM32F412RG --target upload
```

### STM32 — Application

```bash
cd stm32/STM32F412RGT6/application
pio build -e genericSTM32F412RG
pio run -e genericSTM32F412RG --target upload
```

Monitor serial output at **115200 baud**.

### ESP32 / Pico — MicroPython

Copy the contents of `esp32/micropython/` or `pico/micropython/` to the device filesystem using `mpremote`, Thonny, or `rshell`.

```bash
mpremote connect <port> cp -r esp32/micropython/. :
```

The wireless module auto-discovers the STM32 via UART ping and syncs configuration from FRAM on every startup.

---

## Configuration

### FRAM Layout (STM32 / FM24CL16B)

Device configuration is persisted in FRAM and read by the wireless module over UART on startup.

| Address | Content |
|---|---|
| `0x008` | Device ID (used as MQTT client ID) |
| Peripheral flags | EXT_RTC_PRESENT, FLASH_PRESENT, DISPLAY_PRESENT, SHT40_PRESENT, BME280_PRESENT, INA226_PRESENT, ADC_PRESENT, CAN_PRESENT |

### Secrets (NVS)

WiFi credentials, API keys, and MQTT credentials are stored in the NVS partition and are excluded from version control (`.gitignore`).

### Device Info Page

A 256-byte info block is programmed to flash address `0x080FF800` during production, containing:

| Field | Magic |
|---|---|
| Magic number | `0x494E464F` |
| Serial number | — |
| Hardware version | — |
| Production date | — |

---

## Communication Protocol

### STM32 ↔ Wireless Module (UART)

Fixed 32-byte frames:

```
[ 5-byte header ] [ 27-byte payload ] [ footer ]
```

The wireless module sends a ping on startup; the STM32 responds with the current configuration payload.

### MQTT

```
Topic:   devices/{device_id}/status
Payload: JSON with sensor readings and device state
```

`device_id` is read from FRAM address `0x008`.

---

## Data Logging

Sensor readings are stored in a circular buffer on the SPI NOR Flash (MX25L25673G).

Each record contains:

| Field | Description |
|---|---|
| Sequence number | Monotonic counter |
| Timestamp | Unix timestamp from RTC |
| Temperature | °C |
| Humidity | % RH |
| Pressure | hPa |
| CRC32 | Frame integrity check |

The flash driver supports retrieval of the oldest and latest records. Logging occurs on a **10-minute interval**; the LCD refreshes every **1 second**.

---

## Production Workflow

1. Flash the bootloader to `0x08000000`
2. Flash the application to `0x08008000`
3. Generate the device info page binary:
   ```bash
   python stm32/production_files/production_release.py
   ```
4. Flash `info_page.bin` to `0x080FF800`
5. Upload MicroPython firmware to the wireless module
6. Copy application scripts and configure secrets via NVS

---

## Firmware Versions

| Component | Version |
|---|---|
| STM32 Application | 0.0.1 |
| STM32 Bootloader | 0.0.1 |
