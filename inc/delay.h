/*
 * Delay functions.
 */

#ifndef DELAY_H
#define DELAY_H

/* Millisecond delay. Implemented in platform.c */
void delay_ms(unsigned int ms);

/* Get raw systick valua */
unsigned int systick_get_raw();
unsigned int systick_get_hz();

#endif
