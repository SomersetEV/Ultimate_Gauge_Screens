# Temps screen

Built on Garage Tinkering's [Ultimate_Gauge_Board_PIO](https://github.com/garagetinkering/Ultimate_Gauge_Board_PIO).

Firmware for the Garage Tinkering Ultimate Gauge Board (ESP32-S3, 2.1" 480x480 ST7701 display), built with PlatformIO, Arduino and LVGL 9.3.

A list of drivetrain temperatures and battery cell voltages. The heater row only appears while the heater is drawing power.

## CAN (500 kbps, 11-bit IDs)

| ID | Data | Shown as |
|----|------|----------|
| 0x126 | bytes 4-5 int16 LE inverter °C, bytes 6-7 int16 LE motor °C | Motor, Inverter |
| 0x373 | bytes 0-1 min cell mV, 2-3 max cell mV, 4-5 min temp K, 6-7 max temp K (all uint16 LE) | Cell Low, Cell High, Battery (average of min/max) |
| 0x802 | bytes 2-3, uint16 LE, W | Heater (hidden at 0 W) |
| 0x801 | byte 0 bit 1 = sidelights | Dims the backlight |
| TBD | charge mode (placeholder in `Process_ChargeMode`) | Backlight off |

## Startup

The screen stays dark until 5.8 s, then the backlight fades in over 0.5 s.

## Code layout

- `src/main.cpp`: CAN task, decoding, backlight.
- `src/ui/ui.c` / `ui.h`: the LVGL screen, written by hand. The setters only redraw when a value changes.
- `src/ui/ui_font_*.c`: fonts generated with the LVGL font converter.
- `lib/`: board drivers (display, LVGL glue, CAN, I2C, TCA9554 IO expander).
