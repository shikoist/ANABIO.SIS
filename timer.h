//timer.h - header for system timer (modified for TMU memory test)

#ifndef _TIMER_H
#define _TIMER_H

// Used interrupt
//#define TIMER_INTERRUPT 0x1C
#define TIMER_INTERRUPT  0x08

// Frequency of standard timer
#define BASE_FREQUENCY  1193182UL

// How many times more frequently the timer will trigger
#define MULTIPLIER 64

// BASE_FREQUENCY / DOS_DIVIDER = ~18.2 ticks/second (standard)
#define DOS_DIVIDER 65535UL

#define CUSTOM_DIVIDER DOS_DIVIDER / MULTIPLIER

extern volatile unsigned long dos_time;  // current counter value

void set_timer_divider(unsigned long divider);
void timer_shutdown();
void timer_init();

#endif
