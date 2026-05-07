#include "linux_internal.h"
#include "sbgl_types.h"
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <string.h>

static SBGL_Scancode x11_keysym_to_scancode(KeySym keysym) {
    if (keysym >= 'a' && keysym <= 'z') return (SBGL_Scancode)(SBGL_SCANCODE_A + (keysym - 'a'));
    if (keysym >= 'A' && keysym <= 'Z') return (SBGL_Scancode)(SBGL_SCANCODE_A + (keysym - 'A'));
    if (keysym == '0') return SBGL_SCANCODE_0;
    if (keysym >= '1' && keysym <= '9') return (SBGL_Scancode)(SBGL_SCANCODE_1 + (keysym - '1'));

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
    sbgl_InputState* input = window->input;
    switch (event->type) {
        case KeyPress: {
            KeySym keysym = XLookupKeysym(&event->xkey, 0);
            SBGL_Scancode code = x11_keysym_to_scancode(keysym);
            if (code < SBGL_SCANCODE_MAX) {
                if (!input->keysDown[code]) input->keysPressed[code] = true;
                input->keysDown[code] = true;
            }
            break;
        }
        case KeyRelease: {
            // X11 Auto-repeat handling
            if (XEventsQueued(window->display, QueuedAfterReading)) {
                XEvent next;
                XPeekEvent(window->display, &next);
                if (next.type == KeyPress && next.xkey.time == event->xkey.time && next.xkey.keycode == event->xkey.keycode) {
                    XNextEvent(window->display, event); // Consume repeat
                    break;
                }
            }
            KeySym keysym = XLookupKeysym(&event->xkey, 0);
            SBGL_Scancode code = x11_keysym_to_scancode(keysym);
            if (code < SBGL_SCANCODE_MAX) input->keysDown[code] = false;
            break;
        }
        case MotionNotify: {
            input->mouseX = event->xmotion.x;
            input->mouseY = event->xmotion.y;
            break;
        }
        case ButtonPress:
        case ButtonRelease: {
            bool down = (event->type == ButtonPress);
            int btn = -1;
            if (event->xbutton.button == Button1) btn = SBGL_MOUSE_BUTTON_LEFT;
            else if (event->xbutton.button == Button3) btn = SBGL_MOUSE_BUTTON_RIGHT;
            else if (event->xbutton.button == Button2) btn = SBGL_MOUSE_BUTTON_MIDDLE;
            if (btn != -1 && btn < SBGL_MOUSE_BUTTON_MAX) input->mouseDown[btn] = down;
            break;
        }
        case FocusIn: {
            window->focused = true;
            break;
        }
        case FocusOut: {
            window->focused = false;
            break;
        }
    }
}

void linux_internal_update_input_states(sbgl_Window* window) {
    sbgl_InputState* input = window->input;
    if (!input) return;
    input->mouseDeltaX = input->mouseX - input->_internalMouseX;
    input->mouseDeltaY = input->mouseY - input->_internalMouseY;
    input->_internalMouseX = input->mouseX;
    input->_internalMouseY = input->mouseY;
}
