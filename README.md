# Ultimate Gauge Screens

Three screens for the **[Garage Tinkering](https://github.com/garagetinkering) Ultimate Gauge Board**: an ESP32-S3 with a 2.1" 480x480 round ST7701 display and a CAN transceiver.

> **Based on [garagetinkering/Ultimate_Gauge_Board_PIO](https://github.com/garagetinkering/Ultimate_Gauge_Board_PIO).**
> Each project here started from that PlatformIO base project. The board drivers in each `lib/` folder (display, LVGL glue, CAN, I2C, TCA9554 IO expander) come from it.

Each screen is a separate PlatformIO project and gets flashed to its own board.

## Screens

| Screen | Shows | CAN IDs |
|--------|-------|---------|
| [Power](Ultimate_Gauge_Board_PIO_Power/README.md) | Battery current (A) and power (kW) arcs from an ISA shunt. Blue below zero, green above | 0x521, 0x526 |
| [SOC](Ultimate_Gauge_Board_PIO_SOC/README.md) | State-of-charge arc (red below 20%) and system voltage | 0x355, 0x802 |
| [Temps](Ultimate_Gauge_Board_PIO_Temps/README.md) | Motor, inverter and battery temps, heater power, and high/low cell voltages | 0x126, 0x373, 0x802 |

Each project's README lists the exact bytes and scaling it decodes.

## Shared behaviour

- **CAN:** 500 kbps with 11-bit IDs, read only.
- **Sidelights:** 0x801 byte 0 bit 1 dims the backlight.
- **Startup:** the screen stays dark until 5.8 s, then the backlight fades in over 0.5 s. Power and SOC then run a full-scale needle sweep.
- **Charge mode:** a placeholder turns the screen off while charging. The CAN ID is still TBD; see `Process_ChargeMode` in `main.cpp`.

## Build and flash

You need [PlatformIO](https://platformio.org/), either the VS Code extension or the CLI. LVGL 9.3 is pulled in automatically through `lib_deps`.

Open one project folder in VS Code and use the PlatformIO Upload button, or run this from the repo root:

```sh
pio run -d Ultimate_Gauge_Board_PIO_Power -t upload
pio run -d Ultimate_Gauge_Board_PIO_SOC   -t upload
pio run -d Ultimate_Gauge_Board_PIO_Temps -t upload
```

## Code layout (per project)

- `src/main.cpp`: CAN task, decoding, backlight and startup animation.
- `src/ui/ui.c` / `ui.h`: the LVGL screen, written by hand. The setters only redraw when a value changes.
- `src/ui/ui_font_*.c`, `ui_image_logo.c`: assets generated with the LVGL converters.
- `lib/`: board drivers from the upstream base project.

## Credits

The base project and board drivers are by Andy Valentine / [Garage Tinkering](https://github.com/garagetinkering/Ultimate_Gauge_Board_PIO). The upstream repository has no license, so this repository is kept private.
