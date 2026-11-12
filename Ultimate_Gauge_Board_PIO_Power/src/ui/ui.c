#include "ui.h"
#include <stdio.h>

#define COLOR_ARC_BG     0x1a2a1a
#define COLOR_ARC_IDLE   0xd7dbd7   /* before the first sign change */
#define COLOR_NEGATIVE   0x2196F3   /* blue: regen / charging */
#define COLOR_POSITIVE   0x4CAF50   /* green: discharging */

typedef struct {
    lv_obj_t  *arc;
    lv_obj_t  *label;
    int32_t    last;
    const char *fmt;
} gauge_t;

static gauge_t s_amps = { NULL, NULL, UI_AMPS_MIN, "%ldA" };
static gauge_t s_kw   = { NULL, NULL, UI_KW_MIN,   "%ldkW" };

static lv_obj_t *create_arc(lv_obj_t *parent, int32_t min, int32_t max) {
    lv_obj_t *arc = lv_arc_create(parent);
    lv_obj_set_pos(arc, 35, 35);
    lv_obj_set_size(arc, 410, 410);
    lv_arc_set_range(arc, min, max);   /* value starts at min */
    lv_arc_set_bg_angles(arc, 210, 330);
    lv_obj_remove_flag(arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_width(arc, 20, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, lv_color_hex(COLOR_ARC_BG), LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, 20, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc, lv_color_hex(COLOR_ARC_IDLE), LV_PART_INDICATOR);
    lv_obj_set_style_line_opa(arc, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(arc, true, LV_PART_INDICATOR);
    return arc;
}

static lv_obj_t *create_label(lv_obj_t *parent, int32_t y, const lv_font_t *font, const char *text) {
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_pos(label, 0, y);
    lv_obj_set_size(label, 480, LV_SIZE_CONTENT);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
    lv_label_set_text(label, text);
    return label;
}

static void gauge_set(gauge_t *g, int32_t value) {
    if (value == g->last) return;

    lv_arc_set_value(g->arc, value);

    /* Recolour only when crossing zero */
    if ((g->last < 0) != (value < 0)) {
        lv_color_t c = lv_color_hex(value < 0 ? COLOR_NEGATIVE : COLOR_POSITIVE);
        lv_obj_set_style_arc_color(g->arc, c, LV_PART_INDICATOR);
        lv_obj_set_style_bg_color(g->arc, c, LV_PART_KNOB);
    }

    lv_label_set_text_fmt(g->label, g->fmt, (long)value);
    g->last = value;
}

void ui_init(void) {
    lv_display_t *disp = lv_display_get_default();
    lv_theme_t *theme = lv_theme_default_init(disp, lv_palette_main(LV_PALETTE_BLUE),
                                              lv_palette_main(LV_PALETTE_RED), true, LV_FONT_DEFAULT);
    lv_display_set_theme(disp, theme);

    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_size(scr, 480, 480);
    lv_obj_set_style_bg_color(scr, lv_color_black(), LV_PART_MAIN);

    /* Order matters: it matches the original z-order */
    s_amps.arc = create_arc(scr, UI_AMPS_MIN, UI_AMPS_MAX);
    s_amps.label = create_label(scr, 87, &ui_font_landroverlarger, "");
    create_label(scr, 149, &ui_font_landroverlarger, "Amps");
    create_label(scr, 285, &ui_font_land_rover, "Power");

    s_kw.arc = create_arc(scr, UI_KW_MIN, UI_KW_MAX);
    lv_arc_set_mode(s_kw.arc, LV_ARC_MODE_REVERSE);
    lv_arc_set_rotation(s_kw.arc, 180);
    s_kw.label = create_label(scr, 339, &ui_font_landroverlarger, "");

    lv_label_set_text_fmt(s_amps.label, s_amps.fmt, (long)s_amps.last);
    lv_label_set_text_fmt(s_kw.label, s_kw.fmt, (long)s_kw.last);

    lv_screen_load(scr);
}

void ui_set_amps(int32_t amps) { gauge_set(&s_amps, amps); }
void ui_set_kw(int32_t kw)     { gauge_set(&s_kw, kw); }
