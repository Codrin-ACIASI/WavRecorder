#include "buttons.h"
#include "hardware/gpio.h"
#include "pico/time.h"


volatile bool flag1 = false;
volatile bool flag2 = false;
volatile bool flag3 = false;
volatile bool flag1_long = false;

void init_button(uint32_t button){
    uint32_t pin_fizic = 0;

    switch (button)
    {
        case select_button:
            pin_fizic = BUTON1;
            break;
        case up_button:
            pin_fizic = BUTON2;
            break;
        case down_button:
            pin_fizic = BUTON3;
            break;
        default:
            return;
    }
    
    gpio_init(pin_fizic);
    gpio_set_dir(pin_fizic, GPIO_IN);
    gpio_pull_up(pin_fizic);
}



void gpio_callback(uint gpio, uint32_t events){
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    
    
    static uint32_t button1_press_time = 0;
    static uint32_t last_interrupt_time = 0;

    
    if (gpio == BUTON1) {
        if (events & GPIO_IRQ_EDGE_FALL) {
            button1_press_time = current_time;
        } 
        else if (events & GPIO_IRQ_EDGE_RISE) {
            uint32_t press_duration = current_time - button1_press_time;
            
            if (press_duration > 50) {
                if (press_duration >= 600) { 
                    flag1_long = true; 
                } else {
                    flag1 = true; 
                }
            }
        }
    } 
    else {
        if(current_time - last_interrupt_time > 200){
            if(gpio == BUTON2){
                flag2 = true;
            }
            if(gpio == BUTON3){
                flag3 = true;
            }
            last_interrupt_time = current_time;
        }
    }
}
