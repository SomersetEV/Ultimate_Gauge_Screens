#include "ui.h"
#include <stdio.h>
#include <limits.h>

#define COLOR_ARC_BG   0x1a2a1a
#define COLOR_NORMAL   0xd7dbd7
#define COLOR_LOW      0xff0000

static lv_obj_t *s_arc;
static lv_obj_t *s_soc_label;
static lv_obj_t *s_volt_label;

static int32_t s_last_soc       = INT32_MIN;
static int32_t s_last_decivolts = INT32_MIN;
static bool    s_last_low       = false;

static lv_obj_t *create_label(lv_obj_t *parent, int32_t y, const lv_font_t *font, const char *text) {
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_pos(label, 0, y);
    lv_obj_set_size(label, 480, LV_SIZE_CONTENT);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
    lv_label_set_text(label, text);
    return label;
}

void ui_init(void) {
    lv_display_t *disp = lv_display_get_default();
    lv_theme_t *theme = lv_theme_default_init(disp, lv_palette_main(LV_PALETTE_BLUE),
                                              lv_palette_main(LV_PALETTE_RED), true, LV_FONT_DEFAULT);
    lv_display_set_theme(disp, theme);

    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_size(scr, 480, 480);
    lv_obj_set_style_bg_color(scr, lv_color_black(), LV_PART_MAIN);

    /* SOC arc, 0-100 */
    s_arc = lv_arc_create(scr);
    lv_obj_set_pos(s_arc, 35, 35);
    lv_obj_set_size(s_arc, 410, 410);
    lv_arc_set_range(s_arc, 0, 100);
    lv_arc_set_bg_angles(s_arc, 210, 330);
    lv_obj_remove_flag(s_arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_width(s_arc, 20, LV_PART_MAIN);
    lv_obj_set_style_arc_color(s_arc, lv_color_hex(COLOR_ARC_BG), LV_PART_MAIN);
    lv_obj_set_style_line_opa(s_arc, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(s_arc, lv_color_hex(COLOR_NORMAL), LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(s_arc, true, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(s_arc, 20, LV_PART_INDICATOR);

    s_soc_label = create_label(scr, 86, &ui_font_landroverlarger, "");
    create_label(scr, 148, &ui_font_landroverlarger, "State of Charge");

    lv_obj_t *logo = lv_image_create(scr);
    lv_obj_set_pos(logo, 140, 269);
    lv_image_set_src(logo, &img_logo);

    s_volt_label = create_label(scr, 221, &ui_font_land_rover, "");

    ui_set_soc(0);
    ui_set_voltage(0.0f);

    lv_screen_load(scr);
}

void ui_set_soc(int32_t percent) {
    if (percent == s_last_soc) return;
    s_last_soc = percent;
    lv_arc_set_value(s_arc, percent);
    lv_label_set_text_fmt(s_soc_label, "%ld%%", (long)percent);
}

void ui_set_soc_low(bool low) {
    if (low == s_last_low) return;
    s_last_low = low;
    lv_color_t c = lv_color_hex(low ? COLOR_LOW : COLOR_NORMAL);
    lv_obj_set_style_arc_color(s_arc, c, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(s_arc, c, LV_PART_KNOB);
}

void ui_set_voltage(float volts) {
    /* Compare in 0.1 V steps so float noise doesn't trigger redraws */
    int32_t dv = (int32_t)(volts * 10.0f + (volts >= 0 ? 0.5f : -0.5f));
    if (dv == s_last_decivolts) return;
    s_last_decivolts = dv;
    char buf[16];
    snprintf(buf, sizeof(buf), "%.1fv", dv / 10.0f);
    lv_label_set_text(s_volt_label, buf);
}
