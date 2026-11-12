# Power gauge

Built on Garage Tinkering's [Ultimate_Gauge_Board_PIO](https://github.com/garagetinkering/Ultimate_Gauge_Board_PIO).

Firmware for the Garage Tinkering Ultimate Gauge Board (ESP32-S3, 2.1" 480x480 ST7701 display), built with PlatformIO, Arduino and LVGL 9.3.

Two arcs show battery current and power from an ISA shunt. Each arc turns blue below zero (regen or charging) and green at zero or above.

## CAN (500 kbps, 11-bit IDs)

| ID | Data | Shown as |
|----|------|----------|
| 0x521 | bytes 2-5, int32 LE, mA (sign inverted) | Amps, -200 to 400 A |
| 0x526 | bytes 2-5, int32 LE, W (sign inverted) | kW, -70 to 300 kW |
| 0x801 | byte 0 bit 1 = sidelights | Dims the backlight |
| TBD | charge mode (placeholder in `Process_ChargeMode`) | Backlight off |

## Startup

The screen stays dark until 5.8 s, then the backlight fades in over 0.5 s. Both arcs then sweep to full scale and back down to the live values.

## Code layout

- `src/main.cpp`: CAN task, decoding, backlight, startup sweep.
- `src/ui/ui.c` / `ui.h`: the LVGL screen, written by hand. The setters only redraw when a value changes.
- `src/ui/ui_font_*.c`: fonts generated with the LVGL font converter.
- `lib/`: board drivers (display, LVGL glue, CAN, I2C, TCA9554 IO expander).

Set `DEBUG_CAN` to 1 in `main.cpp` to print decoded values to Serial.
