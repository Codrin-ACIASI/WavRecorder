#include "ui.h"
#include "lvgl.h"

// Păstrăm un pointer către opt1 ca să o putem modifica mai târziu din exterior
static lv_obj_t *opt1;

void ui_init(void) {
    // fundal titlu
    lv_obj_t * my_rect = lv_obj_create(lv_scr_act());
    lv_obj_set_size(my_rect , 240, 120);
    lv_obj_align(my_rect, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(my_rect , lv_color_hex(0xFFFFFF), 0);
    
    // titlu
    lv_obj_t *symbol = lv_label_create(my_rect);
    lv_obj_set_style_text_font(symbol, &lv_font_montserrat_32, 0);
    lv_label_set_text(symbol, "\xEF\x80\x81");
    lv_obj_set_style_text_color(symbol, lv_color_hex(0x000000), 0);
    lv_obj_align(symbol, LV_ALIGN_TOP_LEFT, 0, 35);

    lv_obj_t *label = lv_label_create(my_rect);
    lv_label_set_text(label, "Echo Note");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0x000000), 0);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 25, 35);

    // optiuni 
    opt1 = lv_label_create(lv_scr_act());
    lv_label_set_text(opt1, "1. Inregistrare voce");
    lv_obj_set_style_text_font(opt1, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(opt1, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(opt1, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *opt2 = lv_label_create(lv_scr_act());
    lv_label_set_text(opt2, "2. Ascultare");
    lv_obj_set_style_text_font(opt2, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(opt2, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(opt2, LV_ALIGN_CENTER, 0, 30);

    lv_obj_t *opt3 = lv_label_create(lv_scr_act());
    lv_label_set_text(opt3, "3. Oprire");
    lv_obj_set_style_text_font(opt3, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(opt3, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(opt3, LV_ALIGN_CENTER, 0, 60);

    // bar
    static lv_style_t style_bg;
    static lv_style_t style_indic;

    lv_style_init(&style_bg);
    lv_style_set_border_color(&style_bg, lv_palette_main(LV_PALETTE_BLUE));
    lv_style_set_border_width(&style_bg, 2);
    lv_style_set_pad_all(&style_bg, 6); 
    lv_style_set_radius(&style_bg, 6);
    lv_style_set_anim_time(&style_bg, 1000);

    lv_style_init(&style_indic);
    lv_style_set_bg_opa(&style_indic, LV_OPA_COVER);
    lv_style_set_bg_color(&style_indic, lv_palette_main(LV_PALETTE_BLUE));
    lv_style_set_radius(&style_indic, 3);

    lv_obj_t * bar = lv_bar_create(lv_scr_act());
    lv_obj_remove_style_all(bar);
    lv_obj_add_style(bar, &style_bg, 0);
    lv_obj_add_style(bar, &style_indic, LV_PART_INDICATOR);

    lv_obj_set_size(bar, 200, 20);
    lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_bar_set_value(bar, 100, LV_ANIM_ON);
}

void ui_set_opt1_active(bool is_active) {
    if(is_active) {
        lv_obj_set_style_text_color(opt1, lv_color_hex(0xFFCCAA), 0);
    } else {
        lv_obj_set_style_text_color(opt1, lv_color_hex(0xFFFFFF), 0);
    }
    // Fortam ecranul sa redeseneze optiunea
    lv_obj_invalidate(opt1);
}