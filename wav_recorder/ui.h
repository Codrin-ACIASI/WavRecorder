#ifndef UI_H_
#define UI_H_

#include "commons.h"
#include"lvgl.h"

typedef enum {
    SCREEN_HOME,
    SCREEN_RECORD_OPTIONS,
    SCREEN_RECORD,
    SCREEN_SLEEP_MODE
} app_screen_t;

void ui_init(void);
void home_screen(void);
void record_screen_options(void);
void record_screen(void);

void ui_set_opt_active(lv_obj_t *opt, bool is_active);



#endif