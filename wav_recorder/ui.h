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

void ui_init(void);
void home_screen(void);
void record_screen_options(void);
void record_screen(void);

void ui_set_opt_active(lv_obj_t *opt, bool is_active);



#endif