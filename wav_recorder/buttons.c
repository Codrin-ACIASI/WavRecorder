#include "buttons.h"
#include "hardware/gpio.h"

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