#include <stdio.h>
#include "pico/stdlib.h"
#include "lvgl.h"
#include "display_port.h"
#include "ui.h"
#include "buttons.h"
#include "hardware/spi.h"

extern lv_timer_t *record_timer;

int main() {
    // 1. Inițializăm consola și oferim un timp de conectare USB
    stdio_init_all();
    sleep_ms(2000); 

    printf("\n--- Start Sistem Echo Note ---\n");

    // 2. Apelăm inițializarea hardware-ului, SD-ului și a interfeței LVGL
    ui_init();

    printf("--- Intrare in bucla principala LVGL ---\n");

    while (true) {
        lv_timer_handler();
        lv_tick_inc(5);
        sleep_ms(5);

        switch(current_screen){
            case SCREEN_HOME: 
                home_screen_logic();
            break;
            case SCREEN_RECORD_OPTIONS:
                intermediate_screen_logic();
            break;
            case SCREEN_LISTEN_MENU:
                listen_menu_logic();
            break;
            case SCREEN_RECORD:
                record_screen_logic();
            break;
            case SCREEN_PLAYBACK:
                playback_logic();
            break;
            case SCREEN_SLEEP_MODE:
               sleep_screen_logic();
            break;
        }
    }
    return 0;
}