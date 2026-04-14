#include <stdio.h>
#include "pico/stdlib.h"
#include "lvgl.h" 
#include "ff.h"
// Modulele facute de noi
#include "display_port.h"
#include "ui.h"
#include "buttons.h"

extern lv_obj_t *opt1;
extern lv_obj_t *opt2;
extern lv_obj_t *opt3;

int main() {
    // 1. Inițializări de bază
    stdio_init_all();
    
    init_button(back_buttton);

    // 2. Pornim modulele
    display_port_init();

    //simbol audio cateva secunde
    ui_init();
    
    uint32_t start = to_ms_since_boot(get_absolute_time());
    while (to_ms_since_boot(get_absolute_time()) - start < 5000) {
        lv_timer_handler();
        lv_tick_inc(5);
        sleep_ms(5);
    }
    
    lv_obj_clean(lv_scr_act());
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x0000FF), 0);
    home_screen();

    // Initializare butoane
    init_button(back_buttton);
    init_button(select_button);
    init_button(up_button);

    // 3. Bucla principală (Logic Loop)
    while (true) {
        // Oferim timp bibliotecii LVGL să animeze
        lv_timer_handler(); 
        lv_tick_inc(5);
        sleep_ms(5); 

        // Citim butonul (apăsat = 0) și actualizăm UI-ul direct!

        bool btn1_pressed = !gpio_get(BUTON1);
        bool btn2_pressed = !gpio_get(BUTON2);
        bool btn3_pressed = !gpio_get(BUTON3);
        
        ui_set_opt_active(opt1, btn1_pressed);
        ui_set_opt_active(opt2, btn2_pressed);
        ui_set_opt_active(opt3, btn3_pressed);
        
    }

    return 0;
}