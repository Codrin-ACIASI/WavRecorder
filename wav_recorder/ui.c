#include "ui.h"
#include "lvgl.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include "buttons.h"
#include "display_port.h"
#include "ff.h"
#include "sd_card.h"

#define MAX_FILES_TO_DISPLAY 10


// Păstrăm un pointer către opt1, opt2 si opt3 ca să o putem modifica mai târziu din exterior
lv_obj_t *opt1;
lv_obj_t *opt2;
lv_obj_t *opt3;

static lv_obj_t * high_label;
static lv_obj_t * low_label;
static int32_t top_num;
static int32_t bottom_num;
static bool update_scroll_running = false;


char wav_files[MAX_FILES_TO_DISPLAY][32]; 
FATFS sd_fs; // Obiectul de sistem de fișiere

volatile home_menu_options current_option_for_home_screen = RECORD_BUTTON;
volatile intermediate_record_screen_options current_option_for_intermediate_screen = START_RECORDING_BUTTON;
volatile record_options current_option_for_record_screen = PAUSE_RECORD_BUTTON;
volatile app_screen_t current_screen = SCREEN_HOME;
volatile playback_options current_option_for_playback_screen = PLAY_PAUSE_BUTTON;

volatile int current_file_index = 0;
volatile int total_files_found = 0;
lv_obj_t *file_list_container =  NULL;

typedef struct {
    lv_obj_t *label;
    uint32_t start_time;
} timer_data_t;

lv_timer_t *record_timer = NULL;
lv_timer_t *playback_timer = NULL;

static inline int safe_modulo(int val, int max_val) {
    return ((val % max_val) + max_val) % max_val;
}

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

static void ui_set_opt_active(lv_obj_t *opt, bool is_active) {

    if(is_active) {
        lv_obj_set_style_bg_color(opt, lv_color_hex(0x9D4EDD), 0);
        lv_obj_set_style_bg_opa(opt, LV_OPA_COVER, 0);

        lv_obj_set_style_text_color(opt, lv_color_hex(0xFFFFFF), 0);

        lv_obj_set_style_pad_left(opt, 6, 0);
        lv_obj_set_style_pad_right(opt, 6, 0);
        lv_obj_set_style_pad_top(opt, 3, 0);
        lv_obj_set_style_pad_bottom(opt, 3, 0);

    } else {
        lv_obj_set_style_bg_opa(opt, LV_OPA_TRANSP, 0);

        lv_obj_set_style_text_color(opt, lv_color_hex(0xB3B3B3), 0);
    }

    lv_obj_invalidate(opt);
}

static void anim_zoom_cb(void *var, int32_t v)
{
    lv_obj_set_style_transform_zoom(var, v, 0);
    lv_obj_set_style_transform_pivot_x(var, lv_obj_get_width(var) / 2, 0);
    lv_obj_set_style_transform_pivot_y(var, lv_obj_get_height(var) / 2, 0);
}

static void initial_focus(){
    ui_set_opt_active(opt1, true);
    ui_set_opt_active(opt2, false);
    ui_set_opt_active(opt3, false);
}



static void change_focus(app_screen_t current_screen, int8_t direction ){
    switch (current_screen)
    {
    case SCREEN_HOME:
        current_option_for_home_screen = safe_modulo(current_option_for_home_screen + direction, 3);
        ui_set_opt_active(opt1, current_option_for_home_screen == RECORD_BUTTON);
        ui_set_opt_active(opt2, current_option_for_home_screen == LISTEN_BUTTON);
        ui_set_opt_active(opt3, current_option_for_home_screen == STOP_BUTTON);
        break;
    case SCREEN_RECORD_OPTIONS:
        current_option_for_intermediate_screen = safe_modulo(current_option_for_intermediate_screen + direction, 2);
        ui_set_opt_active(opt1, current_option_for_intermediate_screen == START_RECORDING_BUTTON);
        ui_set_opt_active(opt2, current_option_for_intermediate_screen == BACK_RECORDING_BUTTON);
    break;
    case SCREEN_RECORD:
        current_option_for_record_screen = safe_modulo(current_option_for_record_screen + direction, 2);
        ui_set_opt_active(opt1, current_option_for_record_screen == SAVE_RECORD_BUTTON);
        ui_set_opt_active(opt2, current_option_for_record_screen == PAUSE_RECORD_BUTTON);
    break;
    }
}

static lv_obj_t *home_screen(void) {
    lv_obj_t* home_scr = lv_obj_create(NULL);

    lv_obj_set_style_bg_color(home_scr, lv_color_hex(0x121212), 0);

    lv_obj_t * my_rect = create_rectangle(home_scr, 240, 120,  LV_ALIGN_TOP_MID, 0,0, 0x9D4EDD);
    lv_obj_set_style_shadow_width(my_rect, 10, 0);
    lv_obj_set_style_shadow_opa(my_rect, LV_OPA_30, 0);

    lv_obj_t *symbol = create_label(my_rect, "\xEF\x80\x81", &lv_font_montserrat_32, 0XFFFFFF, LV_ALIGN_TOP_LEFT, 0, 35);
    lv_obj_t *label = create_label(my_rect, "Echo Note", &lv_font_montserrat_32, 0XFFFFFF, LV_ALIGN_TOP_MID, 25, 35);

    opt1 = create_label(home_scr, "1. Inregistare voce", &lv_font_montserrat_20, 0xB3B3B3, LV_ALIGN_CENTER, 0, 0);
    opt2 = create_label(home_scr, "2. Ascultare", &lv_font_montserrat_20, 0xB3B3B3, LV_ALIGN_CENTER, 0, 30);
    opt3 = create_label(home_scr, "3. Oprire", &lv_font_montserrat_20, 0xB3B3B3, LV_ALIGN_CENTER, 0, 60);

    return home_scr;
}

static lv_obj_t *record_options_screen(void)
{
    lv_obj_t* record_options_scr = lv_obj_create(NULL);

    lv_obj_set_style_bg_color(record_options_scr, lv_color_hex(0x121212), 0);

    lv_obj_t * my_rect = create_rectangle(record_options_scr, 240, 120,  LV_ALIGN_TOP_MID, 0,0, 0x9D4EDD);
    lv_obj_set_style_shadow_width(my_rect, 10, 0);
    lv_obj_set_style_shadow_opa(my_rect, LV_OPA_30, 0);

    lv_obj_t *symbol = create_label(my_rect, "\xEF\x80\x81", &lv_font_montserrat_32, 0XFFFFFF, LV_ALIGN_TOP_LEFT, 0, 35);
    lv_obj_t *label = create_label(my_rect, "Echo Note", &lv_font_montserrat_32, 0XFFFFFF, LV_ALIGN_TOP_MID, 25, 35);

    opt1 = create_label(record_options_scr, "1. Inregistreaza", &lv_font_montserrat_20, 0xB3B3B3, LV_ALIGN_CENTER, 0, 0);
    opt2 = create_label(record_options_scr, "2. Inapoi la pagina\nprincipala", &lv_font_montserrat_20, 0xB3B3B3, LV_ALIGN_CENTER, 0, 50);
    

    return record_options_scr;

}


static void start_symbol(void)
{
    lv_obj_t *symbol = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(symbol, &lv_font_montserrat_32, 0);
    lv_label_set_text(symbol, LV_SYMBOL_AUDIO);
    lv_obj_set_style_text_color(symbol, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(symbol, LV_ALIGN_CENTER, 0, 0);  
}

static void rec_anim_cb(void *obj, int32_t v)
{
    lv_obj_set_style_opa((lv_obj_t *)obj, v, 0);
}

static void update_timer_cb(lv_timer_t *t)
{
    timer_data_t *data = (timer_data_t *)t->user_data;

    uint32_t elapsed = to_ms_since_boot(get_absolute_time()) - data->start_time;

    uint32_t seconds = elapsed / 1000;
    uint32_t minutes = seconds / 60;
    seconds = seconds % 60;

    static char buf[16];
    sprintf(buf, "%02d:%02d", minutes, seconds);

    lv_label_set_text(data->label, buf);
}


static lv_obj_t *record_screen(void)
{
    lv_obj_t *timer_label;
    lv_obj_t *rec_label;
    lv_timer_t *timer;

    lv_obj_t *record_scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(record_scr, lv_color_hex(0x121212), 0);


    rec_label = lv_label_create(record_scr);
    lv_label_set_text(rec_label, "REC");
    lv_obj_align(rec_label, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_set_style_text_color(rec_label, lv_color_hex(0xFF0000), 0);

    lv_obj_t * my_rect = create_rectangle(record_scr, 120, 60,  LV_ALIGN_CENTER, 0,0, 0x9D4EDD);
    lv_obj_set_style_shadow_width(my_rect, 10, 0);
    lv_obj_set_style_shadow_opa(my_rect, LV_OPA_30, 0);

    timer_label = lv_label_create(record_scr);
    lv_label_set_text(timer_label, "00:00");
    lv_obj_align(timer_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_color(timer_label, lv_color_hex(0xFFFFFF), 0);

    timer_data_t *data = lv_mem_alloc(sizeof(timer_data_t));
    data->label = timer_label;
    data->start_time = to_ms_since_boot(get_absolute_time());

    record_timer = lv_timer_create(update_timer_cb, 200, data);

    opt1 = create_label(record_scr, LV_SYMBOL_STOP, &lv_font_montserrat_32, 0xFFFFFF, LV_ALIGN_BOTTOM_MID, 40, -20);
    opt2 = create_label(record_scr, LV_SYMBOL_PAUSE, &lv_font_montserrat_32, 0xFFFFFF, LV_ALIGN_BOTTOM_MID, -40, -20);

    static lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, rec_label);
    lv_anim_set_exec_cb(&a, rec_anim_cb);
    lv_anim_set_values(&a, LV_OPA_100, LV_OPA_20);
    lv_anim_set_time(&a, 500);
    lv_anim_set_playback_time(&a, 500);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&a);    

    return record_scr;
}


static lv_obj_t* listen_menu_screen(){
    lv_obj_t *listen_scr= lv_obj_create(NULL);
    lv_obj_set_style_bg_color(listen_scr, lv_color_hex(0x121212), 0);

    lv_obj_t *header = create_rectangle(listen_scr, 240, 40, LV_ALIGN_TOP_MID, 0, 0, 0x9D4EDD);
    create_label(header, "Fisiere .WAV", &lv_font_montserrat_20, 0xFFFFFF, LV_ALIGN_CENTER, 0, 0);


    file_list_container = lv_obj_create(listen_scr);
    lv_obj_set_size(file_list_container, 240, 280);
    lv_obj_align(file_list_container, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(file_list_container, lv_color_hex(0x121212), 0);
    lv_obj_set_style_border_width(file_list_container, 0, 0);

    lv_obj_set_flex_flow(file_list_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(file_list_container, 5, 0);

    scan_sd_for_wavs();

    if (total_files_found == 0) {
        lv_obj_t* empty_label = lv_label_create(file_list_container);
        lv_label_set_text(empty_label, "Niciun fisier .WAV gasit.");
        lv_obj_set_style_text_color(empty_label, lv_color_hex(0xFF0000), 0); // Rosu
        lv_obj_set_style_pad_all(empty_label, 10, 0);
        return listen_scr;
    }

    for(int i = 0; i < total_files_found; ++i){
        lv_obj_t* item = lv_label_create(file_list_container);
        
        
        char display_name[40];
        sprintf(display_name, " %s", wav_files[i]); 
        lv_label_set_text(item, display_name);

        lv_obj_set_style_text_font(item, &lv_font_montserrat_20, 0);
        lv_obj_set_style_pad_all(item, 10, 0);
        lv_obj_set_width(item, lv_pct(100));

        // Focusul rămâne pe elementul selectat (sau pe 0 dacă abia am intrat)
        if(i == current_file_index) {
            lv_obj_set_style_bg_color(item, lv_color_hex(0x9D4EDD), 0);
            lv_obj_set_style_bg_opa(item, LV_OPA_COVER, 0);
            lv_obj_set_style_text_color(item, lv_color_hex(0xFFFFFF), 0);
        } else {
            lv_obj_set_style_bg_opa(item, LV_OPA_TRANSP, 0);
            lv_obj_set_style_text_color(item, lv_color_hex(0xB3B3B3), 0);
        }
    }

    return listen_scr;

}

static lv_obj_t *playback_screen(int file_index){

    lv_obj_t *timer_label;
    lv_obj_t *filename_label;

    lv_obj_t *play_scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(play_scr, lv_color_hex(0x121212), 0);

    char filename[20];
    sprintf(filename, "REC_%03d.WAV", file_index + 1);

    filename_label = lv_label_create(play_scr);
    lv_label_set_text(filename_label, filename);
    lv_obj_align(filename_label, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_text_color(filename_label, lv_color_hex(0x9D4EDD), 0);
    lv_obj_set_style_text_font(filename_label, &lv_font_montserrat_20, 0);

    lv_obj_t *my_rect = create_rectangle(play_scr, 120, 60, LV_ALIGN_CENTER, 0, 0, 0x9D4EDD);
    lv_obj_set_style_shadow_width(my_rect, 10, 0);
    lv_obj_set_style_shadow_opa(my_rect, LV_OPA_30, 0);

    timer_label = lv_label_create(play_scr);
    lv_label_set_text(timer_label, "00:00");
    lv_obj_align(timer_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_color(timer_label, lv_color_hex(0xFFFFFF), 0);


    timer_data_t *data = lv_mem_alloc(sizeof(timer_data_t));
    data->label = timer_label;
    data->start_time = to_ms_since_boot(get_absolute_time());

    playback_timer = lv_timer_create(update_timer_cb, 200, data);

    opt1 = create_label(play_scr, LV_SYMBOL_PAUSE, &lv_font_montserrat_32, 0xFFFFFF, LV_ALIGN_BOTTOM_MID, -40, -20); 
    opt2 = create_label(play_scr, LV_SYMBOL_STOP, &lv_font_montserrat_32, 0xFFFFFF, LV_ALIGN_BOTTOM_MID, 40, -20);  

    
    return play_scr;
}

static void scan_sd_for_wavs(void){
    DIR dir;
    FILINFO fno;
    FRESULT res;

    total_files_found = 0;

    res = f_mount(&sd_fs, "0:", 1);
    if(res != FR_OK){
        printf("Eroare FatFs: Nu am putut monta cardul SD! (Cod: %d)\n", res);
        return;
    }

    res = f_opendir(&dir, "0:/");
    if(res == FR_OK){
        while (total_files_found < MAX_FILES_TO_DISPLAY){
            res = f_readdir(&dir, &fno);

            if(res != FR_OK || fno.fname[0] == NULL){
                break;
            }

            if(!(fno.fattrib & AM_DIR)){
                int len = strlen(fno.fname);
                if (len > 4 && (strcmp(&fno.fname[len - 4], ".WAV") == 0 || strcmp(&fno.fname[len - 4], ".wav") == 0)){

                    strcpy(wav_files[total_files_found], fno.fname);
                    total_files_found++;
                }
            }
        }
        f_closedir(&dir);
        
    }else{
        printf("Eroare FatFs: Nu am putut deschide directorul principal!\n");
    }
}

void listen_menu_logic(void){
    if(total_files_found == 0){
        return;
    }

    if(flag2){
        flag2 = false;

        lv_obj_t *old_item = lv_obj_get_child(file_list_container, current_file_index);
        lv_obj_set_style_bg_opa(old_item, LV_OPA_TRANSP, 0);
        lv_obj_set_style_text_color(old_item, lv_color_hex(0xB3B3B3), 0);

        current_file_index = safe_modulo(current_file_index + 1, total_files_found);

        lv_obj_t *new_item = lv_obj_get_child(file_list_container, current_file_index);
        lv_obj_set_style_bg_color(new_item, lv_color_hex(0x9D4EDD), 0);
        lv_obj_set_style_bg_opa(new_item, LV_OPA_COVER, 0);
        lv_obj_set_style_text_color(new_item, lv_color_hex(0xFFFFFF), 0);

        lv_obj_scroll_to_view(new_item, LV_ANIM_OFF);
    }

    if (flag3) {
        flag3 = false;
        
        
        lv_obj_t *old_item = lv_obj_get_child(file_list_container, current_file_index);
        lv_obj_set_style_bg_opa(old_item, LV_OPA_TRANSP, 0);
        lv_obj_set_style_text_color(old_item, lv_color_hex(0xB3B3B3), 0);

        
        current_file_index = safe_modulo(current_file_index - 1, total_files_found);

        
        lv_obj_t *new_item = lv_obj_get_child(file_list_container, current_file_index);
        lv_obj_set_style_bg_color(new_item, lv_color_hex(0x9D4EDD), 0);
        lv_obj_set_style_bg_opa(new_item, LV_OPA_COVER, 0);
        lv_obj_set_style_text_color(new_item, lv_color_hex(0xFFFFFF), 0);

        
        lv_obj_scroll_to_view(new_item, LV_ANIM_OFF);
    }
    if (flag1_long) {
        flag1_long = false;
        flag1 = false; 
        
        lv_scr_load_anim(home_screen(), LV_SCR_LOAD_ANIM_NONE, 0, 0, true);
        current_screen = SCREEN_HOME;
        current_option_for_home_screen = RECORD_BUTTON;
        initial_focus(); 
    }

    if (flag1) {
        flag1 = false;
        
        lv_scr_load_anim(playback_screen(current_file_index), LV_SCR_LOAD_ANIM_NONE, 0, 0, true);
        current_option_for_playback_screen = PLAY_PAUSE_BUTTON;
        ui_set_opt_active(opt1, current_option_for_playback_screen == PLAY_PAUSE_BUTTON);
        ui_set_opt_active(opt2, current_option_for_playback_screen == STOP_BACK_BUTTON);
        
        current_screen = SCREEN_PLAYBACK;
    }
}

void home_screen_logic(void){
    if (flag2) {
        flag2 = false; 
        change_focus(current_screen, 1);
    }

    if (flag3) {
        flag3 = false;
        change_focus(current_screen, -1);
    }

    if (flag1) {
        flag1 = false;
        if (current_option_for_home_screen == RECORD_BUTTON) {
        lv_scr_load_anim(record_options_screen(), LV_SCR_LOAD_ANIM_NONE, 0, 0, true);
        current_option_for_intermediate_screen = START_RECORDING_BUTTON;
        ui_set_opt_active(opt1, current_option_for_intermediate_screen == START_RECORDING_BUTTON);
        current_screen = SCREEN_RECORD_OPTIONS;
        }
        else if (current_option_for_home_screen == LISTEN_BUTTON){
            current_file_index = 0;
            lv_scr_load_anim(listen_menu_screen(), LV_SCR_LOAD_ANIM_NONE, 300, 0, true);
            current_screen = SCREEN_LISTEN_MENU;
        }
                 
        else if (current_option_for_home_screen == STOP_BUTTON) {
            lv_obj_clean(lv_scr_act());
            lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x0), 0);
            current_screen = SCREEN_SLEEP_MODE;
        }
    }
}

void intermediate_screen_logic(void){
    if (flag2) {
        flag2 = false;
        change_focus(current_screen, 1);
    }

    if (flag3) {
        flag3 = false;
        change_focus(current_screen, -1);
    }

    if (flag1) {
        flag1 = false;
        if (current_option_for_intermediate_screen == START_RECORDING_BUTTON) {
            lv_scr_load_anim(record_screen(), LV_SCR_LOAD_ANIM_NONE, 0, 0, true);
            current_option_for_record_screen = PAUSE_RECORD_BUTTON;
            ui_set_opt_active(opt1, current_option_for_record_screen == SAVE_RECORD_BUTTON);
            ui_set_opt_active(opt2, current_option_for_record_screen == PAUSE_RECORD_BUTTON);
            current_screen = SCREEN_RECORD;
        } else if (current_option_for_intermediate_screen == BACK_RECORDING_BUTTON) {
            lv_scr_load_anim(home_screen(), LV_SCR_LOAD_ANIM_NONE, 0, 0, true);
            current_screen = SCREEN_HOME;
            current_option_for_home_screen = RECORD_BUTTON;
            ui_set_opt_active(opt1, current_option_for_home_screen == RECORD_BUTTON);
        }
            
    }
}

void record_screen_logic(){
    if (flag2) {
        flag2 = false;
        change_focus(current_screen, 1);
    }

    if (flag3) {
        flag3 = false;
        change_focus(current_screen, -1);
    }

    static bool paused = false;
    if (flag1) {
        flag1 = false;
        if (current_option_for_record_screen == SAVE_RECORD_BUTTON) {
        if (record_timer) {
            lv_timer_del(record_timer);
            record_timer = NULL;
        }
        lv_scr_load_anim(home_screen(), LV_SCR_LOAD_ANIM_NONE, 0, 0, true);
        current_screen = SCREEN_HOME;
        current_option_for_home_screen = RECORD_BUTTON;
        ui_set_opt_active(opt1, current_option_for_home_screen == RECORD_BUTTON);
        paused = false;
        } else if (current_option_for_record_screen == PAUSE_RECORD_BUTTON) {
            paused = !paused;
            if (paused) {
                lv_label_set_text(opt2, LV_SYMBOL_PLAY);
                lv_timer_pause(record_timer);
            } else {
                lv_label_set_text(opt2, LV_SYMBOL_PAUSE);
                lv_timer_resume(record_timer);
            }
        }
    }
}

void playback_logic(void){
    if (flag2) {
        flag2 = false;
        current_option_for_playback_screen = safe_modulo(current_option_for_playback_screen + 1, 2);
        ui_set_opt_active(opt1, current_option_for_playback_screen == PLAY_PAUSE_BUTTON);
        ui_set_opt_active(opt2, current_option_for_playback_screen == STOP_BACK_BUTTON);
    }

    if (flag3) {
        flag3 = false;
        current_option_for_playback_screen = safe_modulo(current_option_for_playback_screen - 1, 2);
        ui_set_opt_active(opt1, current_option_for_playback_screen == PLAY_PAUSE_BUTTON);
        ui_set_opt_active(opt2, current_option_for_playback_screen == STOP_BACK_BUTTON);
    }

    static bool is_playing = true;

    if(flag1){
        flag1 = false;
        if(current_option_for_playback_screen == PLAY_PAUSE_BUTTON){
            is_playing = !is_playing;
            if(is_playing){
                lv_label_set_text(opt1, LV_SYMBOL_PAUSE);
                lv_timer_resume(playback_timer);
            }else{
                lv_label_set_text(opt1, LV_SYMBOL_PLAY);
                lv_timer_pause(playback_timer);
            }
        }else if (current_option_for_playback_screen == STOP_BACK_BUTTON){
            if(playback_timer){
                lv_timer_del(playback_timer);
                playback_timer = NULL;
            }

            lv_scr_load_anim(listen_menu_screen(), LV_SCR_LOAD_ANIM_NONE, 0, 0, true);
            current_screen = SCREEN_LISTEN_MENU;
            is_playing =true;
        }
        

    }

}
void sleep_screen_logic(void){
     if (flag2 || flag3) {
        flag2 = false;
        flag3 = false;
        lv_scr_load_anim(home_screen(), LV_SCR_LOAD_ANIM_NONE, 0, 0, true);
        current_screen = SCREEN_HOME;
        current_option_for_home_screen = RECORD_BUTTON;
    }
                
        flag1 = false; 
}

void ui_init(void){
    stdio_init_all();
    sd_init_driver();
    display_port_init();

    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x121212), 0);
    lv_obj_t* home = home_screen();
    lv_scr_load(home);

    init_button(select_button);
    init_button(down_button);
    init_button(up_button);

    gpio_set_irq_enabled_with_callback(BUTON1, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &gpio_callback);
    gpio_set_irq_enabled(BUTON2, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(BUTON3, GPIO_IRQ_EDGE_FALL, true);

    initial_focus();
}

