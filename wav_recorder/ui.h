#ifndef UI_H_
#define UI_H_

#include "commons.h"
#include"lvgl.h"


extern lv_obj_t *opt1;
extern lv_obj_t *opt2;
extern lv_obj_t *opt3;

typedef enum {
    SCREEN_HOME,
    SCREEN_RECORD_OPTIONS,
    SCREEN_RECORD,
    SCREEN_SLEEP_MODE
} app_screen_t;

typedef enum {
    RECORD_BUTTON,
    LISTEN_BUTTON,
    STOP_BUTTON,
} home_menu_options;

typedef enum{
    START_RECORDING_BUTTON,
    BACK_RECORDING_BUTTON
} intermediate_record_screen_options;

typedef enum{
    PAUSE_RECORD_BUTTON,
    SAVE_RECORD_BUTTON
} record_options;


extern volatile app_screen_t current_screen;
extern volatile home_menu_options current_option_for_home_screen;
extern volatile intermediate_record_screen_options current_option_for_intermediate_screen;
extern volatile record_options current_option_for_record_screen;




void ui_init(void);
void home_screen_logic(void);
void intermediate_screen_logic(void);
void record_screen_logic(void);
void sleep_screen_logic(void);

#endif