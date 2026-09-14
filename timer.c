// timer.c - нам нужно чётко считать фпс
// Для этого программируем Programmable Interval Timer (PIT) 8254 
// (или его совместимый 8253) в реальном режиме x86
// (DOS, OpenWatcom) для срабатывания во много раз чаще.
// У меня это в 64 раза чаще.
// С помощью #define MULTIPLIER 64 в timer.h

// Просто для удобства, а то всё время забываю
// где какой байт
#define LOW_BYTE(w) ((unsigned char)((unsigned short)(w) & 0xFF))
#define HIGH_BYTE(w) ((unsigned char)(((unsigned short)(w) >> 8)) & 0xFF)

#include <conio.h>
#include <dos.h>

#include "timer.h"

void interrupt (*old_timer_interrupt)();

// volatile означает другое поведение для переменной
// с точки зрения компилятора, она может быть изменена
// в любой момент времени
volatile unsigned long dos_time = 0;

void interrupt timer_interrupt_handler() {
    // Нужно вызывать старый обработчик
    // раз в новое количество раз
    if (dos_time % MULTIPLIER == 0) {
        old_timer_interrupt();
    }
    // будем брать это значение в основном рабочем цикле
    dos_time++;

    outp(0x20, 0x20); // Засылаем EOI в PIC, а то может зависнуть
}

// Функция настраивает канал 0 таймера 
// (тот самый, который генерит IRQ 0 — системное прерывание таймера).
void set_timer_divider(unsigned long divider) {
    // Выключаем прерывание, чтобы не словить мусор в процессе
    _disable();

    // Порт 0x43 — это Command Register (регистр управления) PIT.
    // Запись байта в этот порт отправляет control word таймеру.
    // По битам: выбор счётчика, порядок сначала младший байт, режим Mode 2, формат счётчика, 
    // Биты      00 11 010 0 = 0x34
    outp(0x43, 0x34);

    // Порт 0x40 — регистр данных Counter 0.
    // сначала младший байт (LSB) значения делителя.
    outp(0x40, LOW_BYTE(divider));
    outp(0x40, HIGH_BYTE(divider));

    // Включаем прерывания
    _enable();
}

void timer_shutdown() {
    _dos_setvect(TIMER_INTERRUPT, old_timer_interrupt);
    set_timer_divider(0);
}

void timer_init() {
    old_timer_interrupt = _dos_getvect(TIMER_INTERRUPT);
    _dos_setvect(TIMER_INTERRUPT, timer_interrupt_handler);

    set_timer_divider(CUSTOM_DIVIDER);
}


