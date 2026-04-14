#include <stdio.h>
#include "pico/stdlib.h"
#include "lvgl.h" 
#include "ff.h"
#include "display_port.h"
#include "ui.h"
#include "buttons.h"

extern lv_obj_t *opt1;
extern lv_obj_t *opt2;
extern lv_obj_t *opt3;

int main() {
    stdio_init_all();
    
    init_button(back_buttton);

    display_port_init();

    ui_init();
    
    uint32_t start = to_ms_since_boot(get_absolute_time());
    while (to_ms_since_boot(get_absolute_time()) - start < 5000) {
        lv_timer_handler();
        lv_tick_inc(5);
        sleep_ms(5);
    }
    
    lv_obj_clean(lv_scr_act());
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000000), 0);
    home_screen();

    init_button(back_buttton);
    init_button(select_button);
    init_button(up_button);

    int current_option = 0;

    ui_set_opt_active(opt1, true);
    ui_set_opt_active(opt2, false);
    ui_set_opt_active(opt3, false);

    bool last_btn2 = false;
    bool last_btn3 = false;

    while (true) {
        lv_timer_handler(); 
        lv_tick_inc(5);
        sleep_ms(5); 

        bool btn2_pressed = !gpio_get(BUTON2);
        bool btn3_pressed = !gpio_get(BUTON3);

        if(btn2_pressed && !last_btn2) {
            current_option++;
            if(current_option > 2)
                current_option = 0;
        }

        if(btn3_pressed && !last_btn3) {
            current_option--;
            if(current_option < 0)
                current_option = 2;
        }

        ui_set_opt_active(opt1, current_option == 0);
        ui_set_opt_active(opt2, current_option == 1);
        ui_set_opt_active(opt3, current_option == 2);

        last_btn2 = btn2_pressed;
        last_btn3 = btn3_pressed;
    }

    return 0;
}