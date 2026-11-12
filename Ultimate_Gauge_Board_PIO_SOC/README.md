# SOC gauge

Built on Garage Tinkering's [Ultimate_Gauge_Board_PIO](https://github.com/garagetinkering/Ultimate_Gauge_Board_PIO).

Firmware for the Garage Tinkering Ultimate Gauge Board (ESP32-S3, 2.1" 480x480 ST7701 display), built with PlatformIO, Arduino and LVGL 9.3.

An arc shows battery state of charge (red below 20%), with the system voltage and a logo underneath.

## CAN (500 kbps, 11-bit IDs)

| ID | Data | Shown as |
|----|------|----------|
| 0x355 | bytes 0-1, uint16 LE, % | SOC arc and "NN%" |
| 0x802 | bytes 0-1, uint16 LE, mV | "NN.Nv" |
| 0x801 | byte 0 bit 1 = sidelights | Dims the backlight |

## Startup

The screen stays dark until 5.8 s, then the backlight fades in over 0.5 s. The arc then sweeps 0 to 100 to 0% and lands on the live SOC.

## Code layout

- `src/main.cpp`: CAN task, decoding, backlight, startup sweep.
- `src/ui/ui.c` / `ui.h`: the LVGL screen, written by hand. The setters only redraw when a value changes.
- `src/ui/ui_font_*.c`, `ui_image_logo.c`: assets generated with the LVGL converters.
- `lib/`: board drivers (display, LVGL glue, CAN, I2C, TCA9554 IO expander).

Set `DEBUG_CAN` to 1 in `main.cpp` to print decoded values to Serial.
