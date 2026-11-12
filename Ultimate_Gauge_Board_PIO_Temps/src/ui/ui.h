#pragma once

/* Temps screen UI: motor/inverter/battery temps, heater power, cell voltages.
 * Hand-written LVGL. Every setter skips work when the value hasn't changed,
 * so calling them on every CAN frame only redraws on real changes. */

#include <lvgl.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

LV_FONT_DECLARE(ui_font_land_rover);

void ui_init(void);
void ui_set_motor_temp(int32_t c);
void ui_set_inverter_temp(int32_t c);
void ui_set_battery_temp(int32_t c);
void ui_set_heater_w(int32_t w);     /* heater row hidden when w <= 0 */
void ui_set_cell_high_mv(int32_t mv);
void ui_set_cell_low_mv(int32_t mv);

#ifdef __cplusplus
}
#endif
