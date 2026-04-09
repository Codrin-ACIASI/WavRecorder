#ifndef BUTTONS_H_
#define BUTTONS_H_

#include "commons.h"

// --- Pini Butoane ---
#define BUTON1 20
#define BUTON2 22
#define BUTON3 26
#define BUTON4 27

enum my_buttons{
    back_buttton = 0,
    select_button = 1,
    up_button = 2,
    down_button = 3
};



void init_button(uint8_t);

#endif