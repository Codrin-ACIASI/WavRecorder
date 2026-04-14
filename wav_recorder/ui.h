#ifndef UI_H_
#define UI_H_

#include "commons.h"
#include"lvgl.h"

void ui_init(void);
void home_screen(void);

void ui_set_opt_active(lv_obj_t *opt, bool is_active);



#endif