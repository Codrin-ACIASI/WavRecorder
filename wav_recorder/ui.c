#include "ui.h"
#include "lvgl.h"

// Păstrăm un pointer către opt1 ca să o putem modifica mai târziu din exterior


lv_obj_t *opt1;

static lv_obj_t* create_rectangle(lv_obj_t* parent, uint8_t width, uint8_t height, lv_align_t align, uint8_t x_ofs, uint8_t y_ofs, uint32_t color){
    lv_obj_t* new_rectangle = lv_obj_create(parent);
    
    lv_obj_set_size(new_rectangle, width, height);

    lv_obj_align(new_rectangle, align, x_ofs, y_ofs);

    lv_obj_set_style_bg_color(new_rectangle, lv_color_hex(color), 0);

    return new_rectangle;

}

static lv_obj_t* create_progress_bar(lv_obj_t* parent, uint8_t width, uint8_t height,lv_align_t align,uint8_t x_ofs, uint8_t y_ofs,  bool is_animated){
    lv_obj_t * new_bar = lv_bar_create(parent);

    lv_obj_set_size(new_bar, width, height);

    lv_obj_center(new_bar);

    lv_obj_align(new_bar, align, x_ofs, y_ofs);

    if(is_animated)
        lv_bar_set_value(new_bar, 0, LV_ANIM_ON);
    else
        lv_bar_set_value(new_bar, 0, LV_ANIM_OFF);

    return new_bar;
}
static lv_obj_t* create_label(lv_obj_t* parent,  const char* text, const lv_font_t* font ,uint32_t color, lv_align_t align, int16_t x_ofs, int16_t y_ofs){
    
    lv_obj_t *new_label = lv_label_create(parent);    
    lv_label_set_text(new_label, text);

    if(font != NULL){
        lv_obj_set_style_text_font(new_label, font, 0);
    }

    lv_obj_set_style_text_color(new_label, lv_color_hex(color), 0);
    lv_obj_align(new_label, align, x_ofs, y_ofs);

    return new_label;

}

static lv_obj_t* create_button(int16_t pos_x, int16_t pos_y, uint16_t width, uint16_t height, uint32_t color ,const char* text){
    lv_obj_t * btn = lv_btn_create(lv_scr_act());
    lv_obj_set_pos(btn, pos_x, pos_y);
    lv_obj_set_size(btn, width, height);
    lv_obj_set_style_bg_color(btn , lv_color_hex(color), 0);    

    lv_obj_t* label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_center(label);

    return btn;
}



void ui_init(void) {
    // fundal titlu

    create_rectangle(lv_scr_act(), 240, 120,  LV_ALIGN_TOP_MID, 0,0, 0xFFFFFF);
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

    lv_obj_t *label = create_label(my_rect, "Echo Note", &lv_font_montserrat_32, 0X000000, LV_ALIGN_TOP_MID, 25, 35);

    // optiuni 
    opt1 = create_label(lv_scr_act(), "1. Inregistare voce", &lv_font_montserrat_20, 0XFFFFFF, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *opt2 = create_label(lv_scr_act(), "2. Ascultare", &lv_font_montserrat_20, 0XFFFFF, LV_ALIGN_CENTER, 0, 30);

    lv_obj_t *opt3 = create_label(lv_scr_act(), "3. Oprire", &lv_font_montserrat_20, 0XFFFFF, LV_ALIGN_CENTER, 0, 60);

    lv_obj_t * test_btn = create_button(70, 230, 100, 40, 0xFF0000, "TEST");

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
    lv_obj_invalidate(opt1);
}