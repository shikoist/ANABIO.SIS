// timer.c - we need to accurately count the FPS
// For this, we program the Programmable Interval Timer (PIT) 8254
// (or its compatible 8253) in real mode x86
// (DOS, OpenWatcom) to trigger much more frequently.
// Mine is 64 times more frequent.
// Using #define MULTIPLIER 64 in timer.h

// Just for convenience, so I don't keep forgetting
// where each byte is
#define LOW_BYTE(w) ((unsigned char)((unsigned short)(w) & 0xFF))
#define HIGH_BYTE(w) ((unsigned char)(((unsigned short)(w) >> 8)) & 0xFF)

#include <conio.h>
#include <dos.h>

#include "timer.h"

void interrupt (*old_timer_interrupt)();

// volatile means different behavior for a variable
// from the compiler's perspective, it may be changed
// at any moment in time
volatile unsigned long dos_time = 0;

void interrupt timer_interrupt_handler() {
    // We need to call the old handler
    // once the new number of times
    if (dos_time % MULTIPLIER == 0) {
        old_timer_interrupt();
    }
    // we will take this value in the main working loop
    dos_time++;

    outp(0x20, 0x20); // We send an EOI to PIC, otherwise it may hang
}

// The function configures timer channel 0
// (that very one which generates IRQ 0 - the timer interrupt).
void set_timer_divider(unsigned long divider) {
    // We disable interrupts to avoid catching garbage in the process
    _disable();

    // Port 0x43 is the Command Register (control register) for PIT.
    // Writing a byte to this port sends the control word to the timer.
    // Bits: selector, order least significant byte, mode 2, counter format,
    // Bits      00 11 010 0 = 0x34
    outp(0x43, 0x34);

    // Port 0x40 is Counter 0 register.
    // first the least significant byte (LSB) of the divisor value.
    outp(0x40, LOW_BYTE(divider));
    outp(0x40, HIGH_BYTE(divider));

    // We enable interrupts
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


