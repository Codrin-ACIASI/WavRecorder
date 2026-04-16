#include <stdio.h>
#include "pico/stdlib.h"
#include "lvgl.h"
#include "ff.h"
#include "display_port.h"
#include "ui.h"
#include "buttons.h"
#include "hardware/spi.h"

app_screen_t current_screen = SCREEN_HOME;
extern lv_obj_t *opt1;
extern lv_obj_t *opt2;
extern lv_obj_t *opt3;

int main() {
    stdio_init_all();
    display_port_init();

    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x121212), 0);
    home_screen();

    init_button(select_button);
    init_button(down_button);
    init_button(up_button);

    int current_option = 0;

    ui_set_opt_active(opt1, true);
    ui_set_opt_active(opt2, false);
    ui_set_opt_active(opt3, false);

    bool last_btn1 = false;
    bool last_btn2 = false;
    bool last_btn3 = false;

    bool btn1_pressed = false;
    bool btn2_pressed = false;
    bool btn3_pressed = false;

    while (true) {
        lv_timer_handler();
        lv_tick_inc(5);
        sleep_ms(5);

        btn1_pressed = !gpio_get(BUTON1);
        btn2_pressed = !gpio_get(BUTON2);
        btn3_pressed = !gpio_get(BUTON3);

        if (current_screen == SCREEN_HOME) {
            if (btn2_pressed && !last_btn2) {
                current_option = (current_option + 1) % 3;
            }

            if (btn3_pressed && !last_btn3) {
                current_option = (current_option - 1 + 3) % 3;
            }

            ui_set_opt_active(opt1, current_option == 0);
            ui_set_opt_active(opt2, current_option == 1);
            ui_set_opt_active(opt3, current_option == 2);

            if (btn1_pressed && !last_btn1 && current_option == 0) {
                lv_obj_clean(lv_scr_act());
                lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x121212), 0);
                record_screen_options();
                current_screen = SCREEN_RECORD_OPTIONS;
            }

            if (btn1_pressed && !last_btn1 && current_option == 2) {
                lv_obj_clean(lv_scr_act());
                lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x0), 0);
                current_screen = SCREEN_SLEEP_MODE;
            }
        }

        if (current_screen == SCREEN_RECORD_OPTIONS) {
            if (btn2_pressed && !last_btn2) {
                current_option = (current_option + 1) % 2;
            }

            if (btn3_pressed && !last_btn3) {
                current_option = (current_option - 1 + 2) % 2;
            }

            ui_set_opt_active(opt1, current_option == 0);
            ui_set_opt_active(opt2, current_option == 1);

            if (btn1_pressed && !last_btn1 && current_option == 0) {
                //Trebuie cu intreruperi pentru ca trece direct de la SCREEN_RECORD_OPTIONS la SCREEN_RECORD
                //lv_obj_clean(lv_scr_act());
                //lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x121212), 0);
                //record_screen();
                current_screen = SCREEN_RECORD;
            }

            if (btn1_pressed && !last_btn1 && current_option == 1) {
                lv_obj_clean(lv_scr_act());
                lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x121212), 0);
                home_screen();
                current_screen = SCREEN_HOME;
                current_option = 0;
            }
        }

        if (current_screen == SCREEN_SLEEP_MODE) {
            if ((btn2_pressed && !last_btn2) || (btn3_pressed && !last_btn3)) {
                lv_obj_clean(lv_scr_act());
                lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x121212), 0);
                home_screen();
                current_screen = SCREEN_HOME;
                current_option = 0;
            }
        }

        last_btn1 = btn1_pressed;
        last_btn2 = btn2_pressed;
        last_btn3 = btn3_pressed;
    }
}