// keyboard.c - processing keyboard

#include "keyboard.h"

#include <dos.h>
#include <string.h>

unsigned char key_states[128];
unsigned char key_states_prev[128];
static void (__interrupt __far *old_keyboard)() = NULL;

static int was_extended = 0;

void __interrupt __far keyboard_handler()
{
    unsigned char scancode;
    unsigned char index;

    /* Reading raw scan codes */
    _asm {
        in  al, 0x60
        mov scancode, al
    }

    /* === Processing extended prefix (E0) === */
    if (scancode == 0xE0) {
        was_extended = 1;
        goto ack;
    }
    if (scancode == 0xE1) {          /* Pause/Break — ignoring */
        was_extended = 0;
        goto ack;
    }

    /* Calculating array index (always clean scan code 0x00-0x7F) */
    index = scancode & 0x7F;

    /* If this is an extended key — reset flag after processing */
    if (was_extended) {
        was_extended = 0;
    }

    /* Make / Break */
    if (scancode & 0x80) {
        key_states[index] = 0;   /* released */
        // key_states_prev[index] = 1;   /* released */
    } else {
        key_states[index] = 1;   /* pressed */
        // key_states_prev[index] = 0;   /* pressed */
    }

ack:
    /* Acknowledging keyboard and sending EOI */
    _asm {
        in  al, 0x61
        mov ah, al
        or  al, 0x80
        out 0x61, al
        mov al, ah
        and al, 0x7F
        out 0x61, al

        mov al, 0x20
        out 0x20, al
    }
}

void keyboard_init()
{
    memset(key_states, 0, sizeof(key_states));
    old_keyboard = _dos_getvect(9);
    _dos_setvect(9, keyboard_handler);
}

void keyboard_shutdown()
{
    if (old_keyboard) {
        _dos_setvect(9, old_keyboard);
        old_keyboard = NULL;
    }
}
