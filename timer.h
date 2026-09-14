//timer.h - header for system timer (modified for TMU memory test)

#ifndef _TIMER_H
#define _TIMER_H

// Используемое прерывание
//#define TIMER_INTERRUPT 0x1C
#define TIMER_INTERRUPT  0x08

// Частота стандартного таймера
#define BASE_FREQUENCY  1193182UL

// Во сколько раз чаще будет срабатывать таймер
#define MULTIPLIER 64

// BASE_FREQUENCY / DOS_DIVIDER = ~18.2 тиков/секунду (стандартный)
#define DOS_DIVIDER 65535UL

#define CUSTOM_DIVIDER DOS_DIVIDER / MULTIPLIER

extern volatile unsigned long dos_time;  // текущее значение счётчика

void set_timer_divider(unsigned long divider);
void timer_shutdown();
void timer_init();

#endif
