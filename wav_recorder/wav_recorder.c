#include <stdio.h>
#include "pico/stdlib.h"
#include "lvgl.h" 
#include "ff.h"
// Modulele facute de noi
#include "display_port.h"
#include "ui.h"
#include "buttons.h"


int main() {
    // 1. Inițializări de bază
    stdio_init_all();
    
    init_button(back_buttton);

    // 2. Pornim modulele
    display_port_init();
    ui_init();

    // 3. Bucla principală (Logic Loop)
    while (true) {
        // Oferim timp bibliotecii LVGL să animeze
        lv_timer_handler(); 
        lv_tick_inc(5);
        sleep_ms(5); 
        
        // Citim butonul (apăsat = 0) și actualizăm UI-ul direct!
        bool btn1_pressed = !gpio_get(BUTON1);
        ui_set_opt1_active(btn1_pressed);
    }

    return 0;
}