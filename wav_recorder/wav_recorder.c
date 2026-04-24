#include <stdio.h>
#include "pico/stdlib.h"
#include "lvgl.h"
#include "ff.h"
#include "display_port.h"
#include "ui.h"
#include "buttons.h"
#include "hardware/spi.h"

extern lv_timer_t *record_timer;

int main() {

    ui_init();

while (true) {
        // LVGL își desenează liniștit ecranele
        lv_timer_handler();
        lv_tick_inc(5);
        sleep_ms(5);

        // ==========================================
        // GESTIUNEA LOGICII PE BAZA FLAG-URILOR
        // ==========================================
        
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
}