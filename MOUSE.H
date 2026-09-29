#ifndef MOUSE_H
#define MOUSE_H

#define MOUSE_INIT              0x0000 // AX=FFFF if driver loaded, BX=number of buttons
#define MOUSE_SHOW              0x0001
#define MOUSE_HIDE              0x0002
#define MOUSE_GET_STATE         0x0003
#define MOUSE_SET_POSITION      0x0004

// Ranges
#define MOUSE_HORIZONTAL_RANGE  0x0007
#define MOUSE_VERTICAL_RANGE    0x0008

#define MOUSE_GET_MICKEYS  0x000B

typedef struct {
    int x;              // 0 .. 639
    int y;              // 0 .. 479
    int buttons;        // bits: 0 = left, 1 = right, 2 = middle
    int prev_buttons;
    int available;      // 1 = the driver is available
    int visible;
    int prev_x, prev_y;
    float sensitivity;
    short dx;
    short dy;
    float sense_x;
    float sense_y;
} MouseState;

extern MouseState mouse;

int  mouse_init(void); // Returns 0 on success, -1 if the mouse driver is not found
void mouse_update(void); // To call every frame (before processing scene input)
int  mouse_button_pressed(int button);   // 0 = left, 1 = right, 2 = middle
int  mouse_button_down(int button); // The button is currently pressed
void mouse_show(void);
void mouse_hide(void);
void mouse_shutdown(void); // Before exiting the program

#endif
