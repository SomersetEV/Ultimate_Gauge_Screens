#include "ui.h"
#include <stdbool.h>

/* One "Name ........ value" row: name left, value right-aligned in 300 px */
typedef struct {
    lv_obj_t   *name;
    lv_obj_t   *value;
    int32_t     last;
    const char *fmt;
} row_t;

static row_t s_motor     = { NULL, NULL, 0, "%ldC" };
static row_t s_inverter  = { NULL, NULL, 0, "%ldC" };
static row_t s_battery   = { NULL, NULL, 0, "%ldC" };
static row_t s_heater    = { NULL, NULL, 0, "%ldW" };
static row_t s_cell_high = { NULL, NULL, 0, "%ldmV" };
static row_t s_cell_low  = { NULL, NULL, 0, "%ldmV" };

static bool s_heater_visible = false;

static void create_row(lv_obj_t *parent, row_t *row, int32_t y, const char *name) {
    row->name = lv_label_create(parent);
    lv_obj_set_pos(row->name, 90, y);
    lv_obj_set_style_text_font(row->name, &ui_font_land_rover, LV_PART_MAIN);
    lv_label_set_text(row->name, name);

    row->value = lv_label_create(parent);
    lv_obj_set_pos(row->value, 90, y);
    lv_obj_set_size(row->value, 300, LV_SIZE_CONTENT);
    lv_obj_set_style_text_font(row->value, &ui_font_land_rover, LV_PART_MAIN);
    lv_obj_set_style_text_align(row->value, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    lv_label_set_text_fmt(row->value, row->fmt, (long)row->last);
}

static void row_set(row_t *row, int32_t value) {
    if (value == row->last) return;
    row->last = value;
    lv_label_set_text_fmt(row->value, row->fmt, (long)value);
}

void ui_init(void) {
    lv_display_t *disp = lv_display_get_default();
    lv_theme_t *theme = lv_theme_default_init(disp, lv_palette_main(LV_PALETTE_BLUE),
                                              lv_palette_main(LV_PALETTE_RED), true, LV_FONT_DEFAULT);
    lv_display_set_theme(disp, theme);

    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_size(scr, 480, 480);
    lv_obj_set_style_bg_color(scr, lv_color_black(), LV_PART_MAIN);

    create_row(scr, &s_motor,     76,  "Motor");
    create_row(scr, &s_inverter,  114, "Inverter");
    create_row(scr, &s_battery,   152, "Battery");
    create_row(scr, &s_heater,    222, "Heater");
    create_row(scr, &s_cell_high, 292, "Cell High");
    create_row(scr, &s_cell_low,  352, "Cell Low");

    /* Heater row hidden until power > 0 */
    lv_obj_add_flag(s_heater.name,  LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_heater.value, LV_OBJ_FLAG_HIDDEN);

    lv_screen_load(scr);
}

void ui_set_motor_temp(int32_t c)     { row_set(&s_motor, c); }
void ui_set_inverter_temp(int32_t c)  { row_set(&s_inverter, c); }
void ui_set_battery_temp(int32_t c)   { row_set(&s_battery, c); }
void ui_set_cell_high_mv(int32_t mv)  { row_set(&s_cell_high, mv); }
void ui_set_cell_low_mv(int32_t mv)   { row_set(&s_cell_low, mv); }

void ui_set_heater_w(int32_t w) {
    bool show = w > 0;
    if (show) row_set(&s_heater, w);
    if (show == s_heater_visible) return;
    s_heater_visible = show;
    if (show) {
        lv_obj_remove_flag(s_heater.name,  LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(s_heater.value, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(s_heater.name,  LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_heater.value, LV_OBJ_FLAG_HIDDEN);
    }
}
