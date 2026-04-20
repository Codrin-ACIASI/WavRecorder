#ifndef BUTTONS_H_
#define BUTTONS_H_

#include "commons.h"
#include "pico/stdlib.h"


// --- Pini Butoane ---
#define BUTON1 20
#define BUTON2 22
#define BUTON3 26
#define BUTON4 27


extern volatile bool flag1;
extern volatile bool flag2;
extern volatile bool flag3;

enum my_buttons{
    select_button = 1,
    up_button = 2,
    down_button = 3
};


void gpio_callback(uint gpio,  uint32_t events);

void init_button(uint32_t);

#endif