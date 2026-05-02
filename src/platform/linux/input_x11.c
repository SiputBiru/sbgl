#include "linux_internal.h"
#include "sbgl_types.h"
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <string.h>

typedef struct {
    bool keys[SBGL_SCANCODE_MAX];
} sbgl_KeyboardState;

static sbgl_KeyboardState g_keyboardState = {{0}};
static sbgl_KeyboardState g_prevKeyboardState = {{0}};

static bool g_mouseState[SBGL_MOUSE_BUTTON_MAX] = {0};
static int  g_mouseX = 0, g_mouseY = 0;
static int  g_prevMouseX = 0, g_prevMouseY = 0;

static SBGL_Scancode x11_keysym_to_scancode(KeySym keysym) {
    if (keysym >= 'a' && keysym <= 'z') return (SBGL_Scancode)(SBGL_SCANCODE_A + (keysym - 'a'));
    if (keysym >= 'A' && keysym <= 'Z') return (SBGL_Scancode)(SBGL_SCANCODE_A + (keysym - 'A'));
    if (keysym >= '0' && keysym <= '9') return (SBGL_Scancode)(SBGL_SCANCODE_0 + (keysym - '0'));

    switch (keysym) {
        case XK_Escape:    return SBGL_SCANCODE_ESCAPE;
        case XK_Return:    return SBGL_SCANCODE_RETURN;
        case XK_BackSpace: return SBGL_SCANCODE_BACKSPACE;
        case XK_Tab:       return SBGL_SCANCODE_TAB;
        case XK_space:     return SBGL_SCANCODE_SPACE;
        case XK_Shift_L:   return SBGL_SCANCODE_LSHIFT;
        case XK_Control_L: return SBGL_SCANCODE_LCTRL;
        case XK_Alt_L:     return SBGL_SCANCODE_LALT;
        case XK_Up:        return SBGL_SCANCODE_UP;
        case XK_Down:      return SBGL_SCANCODE_DOWN;
        case XK_Left:      return SBGL_SCANCODE_LEFT;
        case XK_Right:     return SBGL_SCANCODE_RIGHT;
        default:           return SBGL_SCANCODE_UNKNOWN;
    }
}

void x11_internal_process_event(XEvent* event, sbgl_Window* window) {
    (void)window;
    switch (event->type) {
        case KeyPress: {
            KeySym keysym = XLookupKeysym(&event->xkey, 0);
            SBGL_Scancode code = x11_keysym_to_scancode(keysym);
            if (code < SBGL_SCANCODE_MAX) g_keyboardState.keys[code] = true;
            break;
        }
        case KeyRelease: {
            // X11 Auto-repeat handling
            if (XEventsQueued(g_x11_display, QueuedAfterReading)) {
                XEvent next;
                XPeekEvent(g_x11_display, &next);
                if (next.type == KeyPress && next.xkey.time == event->xkey.time && next.xkey.keycode == event->xkey.keycode) {
                    XNextEvent(g_x11_display, event); // Consume repeat
                    break;
                }
            }
            KeySym keysym = XLookupKeysym(&event->xkey, 0);
            SBGL_Scancode code = x11_keysym_to_scancode(keysym);
            if (code < SBGL_SCANCODE_MAX) g_keyboardState.keys[code] = false;
            break;
        }
        case MotionNotify: {
            g_mouseX = event->xmotion.x;
            g_mouseY = event->xmotion.y;
            break;
        }
        case ButtonPress:
        case ButtonRelease: {
            bool down = (event->type == ButtonPress);
            if (event->xbutton.button == Button1) g_mouseState[SBGL_MOUSE_BUTTON_LEFT] = down;
            else if (event->xbutton.button == Button3) g_mouseState[SBGL_MOUSE_BUTTON_RIGHT] = down;
            else if (event->xbutton.button == Button2) g_mouseState[SBGL_MOUSE_BUTTON_MIDDLE] = down;
            break;
        }
    }
}

void linux_internal_update_input_states(void) {
    g_prevKeyboardState = g_keyboardState;
    g_prevMouseX = g_mouseX;
    g_prevMouseY = g_mouseY;
}

bool sbgl_os_IsKeyDown(SBGL_Scancode key) { return (key < SBGL_SCANCODE_MAX) ? g_keyboardState.keys[key] : false; }
bool sbgl_os_IsKeyPressed(SBGL_Scancode key) { return (key < SBGL_SCANCODE_MAX) ? (g_keyboardState.keys[key] && !g_prevKeyboardState.keys[key]) : false; }
bool sbgl_os_IsMouseButtonDown(SBGL_MouseButton btn) { return (btn < SBGL_MOUSE_BUTTON_MAX) ? g_mouseState[btn] : false; }
void sbgl_os_GetMousePos(int* x, int* y) { *x = g_mouseX; *y = g_mouseY; }
void sbgl_os_GetMouseDelta(int* dx, int* dy) { *dx = g_mouseX - g_prevMouseX; *dy = g_mouseY - g_prevMouseY; }
