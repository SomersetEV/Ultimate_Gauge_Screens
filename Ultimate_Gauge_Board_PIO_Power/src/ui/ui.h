#pragma once

/* Power gauge UI: battery current (amps) and power (kW) arcs.
 * Hand-written LVGL. Every setter skips work when the value hasn't changed,
 * so calling them on every CAN frame only redraws on real changes. */

#include <lvgl.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

LV_FONT_DECLARE(ui_font_land_rover);
LV_FONT_DECLARE(ui_font_landroverlarger);

#define UI_AMPS_MIN  (-200)
#define UI_AMPS_MAX  400
#define UI_KW_MIN    (-70)
#define UI_KW_MAX    300

void ui_init(void);
void ui_set_amps(int32_t amps);
void ui_set_kw(int32_t kw);

#ifdef __cplusplus
}
#endif
