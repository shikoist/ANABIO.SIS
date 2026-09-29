#include <stdio.h>
#include <i86.h>
#include <dos.h>

#include "CAMERA.H"
#include "MOUSE.H"

MouseState mouse = {0};

// Internal helpers
static void mouse_int(union REGS *r) {
    int386(0x33, r, r);
}

int mouse_init(void) {
    union REGS r;

    // Driver reset and check for driver presence
    r.w.ax = MOUSE_INIT;
    mouse_int(&r);

    if (r.w.ax == 0) {
        mouse.available = 0;
        printf("[MOUSE] Driver not found\n");
        return -1;
    }

    mouse.available = 1;
    printf("[MOUSE] Driver found, buttons: %d\n", r.w.bx);

    // Setting the horizontal range
    r.w.ax = MOUSE_HORIZONTAL_RANGE;
    r.w.cx = 0;
    r.w.dx = SCREEN_WIDTH - 1;
    //r.w.dx = (SCREEN_WIDTH - 1) * 3;
    mouse_int(&r);

    // Vertical range
    r.w.ax = MOUSE_VERTICAL_RANGE;
    r.w.cx = 0;
    r.w.dx = SCREEN_HEIGHT - 1;
    //r.w.dx = (SCREEN_HEIGHT - 1) * 3;
    mouse_int(&r);

    // Center the cursor
    r.w.ax = MOUSE_SET_POSITION;
    r.w.cx = SCREEN_WIDTH / 2;
    r.w.dx = SCREEN_HEIGHT / 2;
    mouse_int(&r);

    mouse.x = SCREEN_WIDTH / 2;
    mouse.y = SCREEN_HEIGHT / 2;
    mouse.sense_x = SCREEN_WIDTH / 2;
    mouse.sense_y = SCREEN_HEIGHT / 2;
    mouse.buttons = 0;
    mouse.prev_buttons = 0;
    mouse.visible = 0;

    mouse.sensitivity = 0.10f;  // could be in options
    mouse.prev_x = mouse.x;
    mouse.prev_y = mouse.y;

    // Showing cursor
    mouse_show();

    return 0;
}

// Should be called every frame
void mouse_update(void)
{
    union REGS r;

    if (!mouse.available) return;

    r.w.ax = MOUSE_GET_STATE; // Get position and button status
    mouse_int(&r);

    mouse.prev_buttons = mouse.buttons;
    mouse.prev_x = mouse.x;
    mouse.prev_y = mouse.y;

    mouse.x       = r.w.cx;
    mouse.y       = r.w.dx;
    mouse.buttons = r.w.bx & 7; // left + right + middle

    // Now relative counters
    // Does not works in DosBox-X
    // r.w.ax = MOUSE_GET_MICKEYS;
    // mouse_int(&r);
    // r.w.cx = horizontal mickeys
    // r.w.dx = vertical mickeys

    // mouse.dx = (short)r.w.cx;   // signed!
    // mouse.dy = (short)r.w.dx;
    mouse.dx = mouse.x - mouse.prev_x;
    mouse.dy = mouse.y - mouse.prev_y;

    mouse.sense_x += (float)mouse.dx * mouse.sensitivity;
    mouse.sense_y += (float)mouse.dy * mouse.sensitivity;

    if (mouse.sense_x < 0) mouse.sense_x = 0;
    if (mouse.sense_x > SCREEN_WIDTH - 1) mouse.sense_x = SCREEN_WIDTH - 1;
    if (mouse.sense_y < 0) mouse.sense_y = 0;
    if (mouse.sense_y > SCREEN_HEIGHT - 1) mouse.sense_y = SCREEN_HEIGHT - 1;
}

int mouse_button_pressed(int button)
{
    int mask;
    if (!mouse.available || button < 0 || button > 2) return 0;
    mask = 1 << button;
    return (mouse.buttons & mask) && !(mouse.prev_buttons & mask);
}

int mouse_button_down(int button)
{
    if (!mouse.available || button < 0 || button > 2) return 0;
    return (mouse.buttons & (1 << button)) != 0;
}

void mouse_show(void)
{
    union REGS r;
    if (!mouse.available || mouse.visible) return;

    r.w.ax = MOUSE_SHOW;
    mouse_int(&r);
    mouse.visible = 1;
}

void mouse_hide(void)
{
    union REGS r;
    if (!mouse.available || !mouse.visible) return;

    r.w.ax = MOUSE_HIDE;
    mouse_int(&r);
    mouse.visible = 0;
}

void mouse_shutdown(void)
{
    if (!mouse.available) return;
    mouse_hide();
    // Can additionally reset the driver, but it is usually not necessary
}
