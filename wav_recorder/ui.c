#include "ui.h"
#include "lvgl.h"

// Păstrăm un pointer către opt1, opt2 si opt3 ca să o putem modifica mai târziu din exterior
lv_obj_t *opt1;
lv_obj_t *opt2;
lv_obj_t *opt3;

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

static void anim_zoom_cb(void *var, int32_t v)
{
    lv_obj_set_style_transform_zoom(var, v, 0);
    lv_obj_set_style_transform_pivot_x(var, lv_obj_get_width(var) / 2, 0);
    lv_obj_set_style_transform_pivot_y(var, lv_obj_get_height(var) / 2, 0);
}

void ui_init(void)
{
    lv_obj_t *symbol = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(symbol, &lv_font_montserrat_32, 0);
    lv_label_set_text(symbol, LV_SYMBOL_AUDIO);
    lv_obj_set_style_text_color(symbol, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(symbol, LV_ALIGN_CENTER, 0, 0);  
}

void home_screen(void) {
    // titlu 
    lv_obj_t * my_rect = create_rectangle(lv_scr_act(), 240, 120,  LV_ALIGN_TOP_MID, 0,0, 0x9D4EDD);
    lv_obj_set_style_shadow_width(my_rect, 10, 0);
    lv_obj_set_style_shadow_opa(my_rect, LV_OPA_30, 0);

    lv_obj_t *symbol = create_label(my_rect, "\xEF\x80\x81", &lv_font_montserrat_32, 0XFFFFFF, LV_ALIGN_TOP_LEFT, 0, 35);
    lv_obj_t *label = create_label(my_rect, "Echo Note", &lv_font_poppins_32, 0XFFFFFF, LV_ALIGN_TOP_MID, 25, 35);

    // optiuni 
    opt1 = create_label(lv_scr_act(), "1. Inregistare voce", &lv_font_montserrat_20, 0xB3B3B3, LV_ALIGN_CENTER, 0, 0);
    opt2 = create_label(lv_scr_act(), "2. Ascultare", &lv_font_montserrat_20, 0xB3B3B3, LV_ALIGN_CENTER, 0, 30);
    opt3 = create_label(lv_scr_act(), "3. Oprire", &lv_font_montserrat_20, 0xB3B3B3, LV_ALIGN_CENTER, 0, 60);
}

void record_screen_options(void)
{
     // titlu 
    lv_obj_t * my_rect = create_rectangle(lv_scr_act(), 240, 120,  LV_ALIGN_TOP_MID, 0,0, 0x9D4EDD);
    lv_obj_set_style_shadow_width(my_rect, 10, 0);
    lv_obj_set_style_shadow_opa(my_rect, LV_OPA_30, 0);

    lv_obj_t *symbol = create_label(my_rect, "\xEF\x80\x81", &lv_font_montserrat_32, 0XFFFFFF, LV_ALIGN_TOP_LEFT, 0, 35);
    lv_obj_t *label = create_label(my_rect, "Echo Note", &lv_font_poppins_32, 0XFFFFFF, LV_ALIGN_TOP_MID, 25, 35);

    opt1 = create_label(lv_scr_act(), "1. Inregistreaza", &lv_font_montserrat_20, 0xB3B3B3, LV_ALIGN_CENTER, 0, 0);
    opt2 = create_label(lv_scr_act(), "2. Inapoi la pagina\nprincipala", &lv_font_montserrat_20, 0xB3B3B3, LV_ALIGN_CENTER, 0, 50);
}

void record_screen(void)
{
    lv_obj_t * my_rect = create_rectangle(lv_scr_act(), 240, 120,  LV_ALIGN_TOP_MID, 0,0, 0x9D4EDD);
    lv_obj_set_style_shadow_width(my_rect, 10, 0);
    lv_obj_set_style_shadow_opa(my_rect, LV_OPA_30, 0);
}

void ui_set_opt_active(lv_obj_t *opt, bool is_active) {

    if(is_active) {
        // fundal alb pe label (ca hover)
        lv_obj_set_style_bg_color(opt, lv_color_hex(0x9D4EDD), 0);
        lv_obj_set_style_bg_opa(opt, LV_OPA_COVER, 0);

        // text negru
        lv_obj_set_style_text_color(opt, lv_color_hex(0xFFFFFF), 0);

        // padding ca sa nu fie lipit textul de margine
        lv_obj_set_style_pad_left(opt, 6, 0);
        lv_obj_set_style_pad_right(opt, 6, 0);
        lv_obj_set_style_pad_top(opt, 3, 0);
        lv_obj_set_style_pad_bottom(opt, 3, 0);

    } else {
        // eliminare fundal
        lv_obj_set_style_bg_opa(opt, LV_OPA_TRANSP, 0);

        // text alb
        lv_obj_set_style_text_color(opt, lv_color_hex(0xB3B3B3), 0);
    }

    lv_obj_invalidate(opt);
}