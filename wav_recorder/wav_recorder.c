#include <stdio.h>
#include "pico/stdlib.h"
#include "lvgl.h"
#include "ff.h"
#include "display_port.h"
#include "ui.h"
#include "buttons.h"
#include "hardware/spi.h"

app_screen_t current_screen = SCREEN_HOME;
extern lv_timer_t *record_timer;




int main() {
    stdio_init_all();
    display_port_init();

    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x121212), 0);
    home_screen();

    init_button(select_button);
    init_button(down_button);
    init_button(up_button);

    gpio_set_irq_enabled_with_callback(BUTON1, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    gpio_set_irq_enabled(BUTON2, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(BUTON3, GPIO_IRQ_EDGE_FALL, true);

    home_menu_options current_option_for_home_screen = RECORD_BUTTON;
    intermediate_record_screen_options current_option_for_intermediate_screen = START_RECORDING_BUTTON;
    record_options current_option_for_record_screen = PAUSE_RECORD_BUTTON;


    ui_set_opt_active(opt1, true);
    ui_set_opt_active(opt2, false);
    ui_set_opt_active(opt3, false);


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
                    if (flag2) {
                    flag2 = false; // "Coborâm" stegulețul
                    current_option_for_home_screen = (current_option_for_home_screen + 1) % 3;
                    ui_set_opt_active(opt1, current_option_for_home_screen == RECORD_BUTTON);
                    ui_set_opt_active(opt2, current_option_for_home_screen == LISTEN_BUTTON);
                    ui_set_opt_active(opt3, current_option_for_home_screen == STOP_BUTTON);
            }

                if (flag3) {
                    flag3 = false;
                    current_option_for_home_screen = (current_option_for_home_screen - 1 + 3) % 3;
                    ui_set_opt_active(opt1, current_option_for_home_screen == RECORD_BUTTON);
                    ui_set_opt_active(opt2, current_option_for_home_screen == LISTEN_BUTTON);
                    ui_set_opt_active(opt3, current_option_for_home_screen == STOP_BUTTON);
            }

                if (flag1) {
                    flag1 = false;
                    if (current_option_for_home_screen == RECORD_BUTTON) {
                    lv_obj_clean(lv_scr_act());
                    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x121212), 0);
                    record_screen_options();
                    current_option_for_intermediate_screen = START_RECORDING_BUTTON;
                    ui_set_opt_active(opt1, current_option_for_intermediate_screen == START_RECORDING_BUTTON);
                    current_screen = SCREEN_RECORD_OPTIONS;
                } else if (current_option_for_home_screen == STOP_BUTTON) {
                    lv_obj_clean(lv_scr_act());
                    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x0), 0);
                    current_screen = SCREEN_SLEEP_MODE;
                }
            }
            break;
            case SCREEN_RECORD_OPTIONS:
                if (flag2) {
                flag2 = false;
                current_option_for_intermediate_screen = (current_option_for_intermediate_screen + 1) % 2;
                ui_set_opt_active(opt1, current_option_for_intermediate_screen == START_RECORDING_BUTTON);
                ui_set_opt_active(opt2, current_option_for_intermediate_screen == BACK_RECORDING_BUTTON);
            }

            if (flag3) {
                flag3 = false;
                current_option_for_intermediate_screen = (current_option_for_intermediate_screen - 1 + 2) % 2;
                ui_set_opt_active(opt1, current_option_for_intermediate_screen == START_RECORDING_BUTTON);
                ui_set_opt_active(opt2, current_option_for_intermediate_screen == BACK_RECORDING_BUTTON);
            }

            if (flag1) {
                flag1 = false;
                if (current_option_for_intermediate_screen == START_RECORDING_BUTTON) {
                    lv_obj_clean(lv_scr_act());
                    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x121212), 0);
                    record_screen();
                    current_option_for_record_screen = PAUSE_RECORD_BUTTON;
                    ui_set_opt_active(opt1, current_option_for_record_screen == SAVE_RECORD_BUTTON);
                    ui_set_opt_active(opt2, current_option_for_record_screen == PAUSE_RECORD_BUTTON);
                    current_screen = SCREEN_RECORD;
                } else if (current_option_for_intermediate_screen == BACK_RECORDING_BUTTON) {
                    lv_obj_clean(lv_scr_act());
                    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x121212), 0);
                    home_screen();
                    current_screen = SCREEN_HOME;
                    current_option_for_home_screen = RECORD_BUTTON;
                    ui_set_opt_active(opt1, current_option_for_home_screen == RECORD_BUTTON);
                }
            }
            break;
            case SCREEN_RECORD:
                        
                if (flag2) {
                    flag2 = false;
                    current_option_for_record_screen = (current_option_for_record_screen + 1) % 2;
                    ui_set_opt_active(opt1, current_option_for_record_screen == SAVE_RECORD_BUTTON);
                    ui_set_opt_active(opt2, current_option_for_record_screen == PAUSE_RECORD_BUTTON);
                }

                if (flag3) {
                    flag3 = false;
                    current_option_for_record_screen = (current_option_for_record_screen - 1 + 2) % 2;
                    ui_set_opt_active(opt1, current_option_for_record_screen == SAVE_RECORD_BUTTON);
                    ui_set_opt_active(opt2, current_option_for_record_screen == PAUSE_RECORD_BUTTON);
                }

                static bool paused = false;
            
                if (flag1) {
                    flag1 = false;
                    if (current_option_for_record_screen == SAVE_RECORD_BUTTON) {
                        if (record_timer) {
                            lv_timer_del(record_timer);
                            record_timer = NULL;
                        }
                        lv_obj_clean(lv_scr_act());
                        lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x121212), 0);
                        home_screen();
                        current_screen = SCREEN_HOME;
                        current_option_for_home_screen = RECORD_BUTTON;
                        ui_set_opt_active(opt1, current_option_for_home_screen == RECORD_BUTTON);
                        paused = false;
                    } else if (current_option_for_record_screen == PAUSE_RECORD_BUTTON) {
                        paused = !paused;
                        if (paused) {
                            lv_label_set_text(opt2, LV_SYMBOL_PLAY);
                            lv_timer_pause(record_timer);
                        } else {
                            lv_label_set_text(opt2, LV_SYMBOL_PAUSE);
                            lv_timer_resume(record_timer);
                        }
                    }
                }
            break;
            case SCREEN_SLEEP_MODE:
                if (flag2 || flag3) {
                    flag2 = false;
                    flag3 = false;
                    // Dacă oricare buton de direcție este apăsat, ieșim din Sleep
                    lv_obj_clean(lv_scr_act());
                    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x121212), 0);
                    home_screen();
                    current_screen = SCREEN_HOME;
                    current_option_for_home_screen = RECORD_BUTTON;
            }
                // Curățăm preventiv și butonul de select ca să nu facă acțiuni neașteptate la trezire
                flag1 = false; 
            break;
        }
            
    }
}