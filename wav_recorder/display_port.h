#ifndef DISPLAY_PORT_H_
#define DISPLAY_PORT_H_
#include <stdint.h>


void display_port_init(void);
uint32_t custom_tick_get(void);
void display_sleep(void);
void display_wake(void);
#endif
