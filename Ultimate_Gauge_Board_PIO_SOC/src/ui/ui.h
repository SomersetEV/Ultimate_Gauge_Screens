#pragma once

/* SOC gauge UI: state-of-charge arc, percentage and system voltage.
 * Hand-written LVGL. Every setter skips work when the value hasn't changed,
 * so calling them on every CAN frame only redraws on real changes. */

#include <lvgl.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

LV_FONT_DECLARE(ui_font_land_rover);
LV_FONT_DECLARE(ui_font_landroverlarger);
LV_IMAGE_DECLARE(img_logo);

void ui_init(void);
void ui_set_soc(int32_t percent);   /* arc + "NN%" label */
void ui_set_soc_low(bool low);      /* red arc/knob when true */
void ui_set_voltage(float volts);   /* "NN.Nv" label */

#ifdef __cplusplus
}
#endif
