#include <stdio.h>
#include "pico/stdlib.h"
#include "lvgl.h"
#include "display_port.h"
#include "ui.h"
#include "buttons.h"
#include "hardware/spi.h"
#include "audio_player.h"

extern lv_timer_t *record_timer;

int main() {
    ui_init();
    audio_init();

    printf("--- Intrare in bucla principala LVGL ---\n");

    uint32_t last_tick = to_ms_since_boot(get_absolute_time());

    while (true) {
        audio_task();
        audio_task();

        uint32_t now = to_ms_since_boot(get_absolute_time());
        if (now - last_tick >= 5) {
            lv_tick_inc(5);
            lv_timer_handler();
            last_tick = now;
        }

        switch (current_screen) {
            case SCREEN_HOME:
                home_screen_logic();
                break;
            case SCREEN_RECORD_OPTIONS:
                intermediate_screen_logic();
                break;
            case SCREEN_RECORD:
                record_screen_logic();
                break;
            case SCREEN_LISTEN_MENU:
                listen_menu_logic();
                break;
            case SCREEN_PLAYBACK:
                playback_logic();
                break;
            case SCREEN_SLEEP_MODE:
                sleep_screen_logic();
                break;
        }

        sleep_us(200);
    }

    return 0;
}