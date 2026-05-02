#include "win32_internal.h"
#include <string.h>

typedef struct {
    bool keys[SBGL_SCANCODE_MAX];
} sbgl_KeyboardState;

static sbgl_KeyboardState g_keyboardState = {{0}};
static sbgl_KeyboardState g_prevKeyboardState = {{0}};

static bool g_mouseState[SBGL_MOUSE_BUTTON_MAX] = {0};
static int  g_mouseX = 0, g_mouseY = 0;
static int  g_prevMouseX = 0, g_prevMouseY = 0;

static SBGL_Scancode win32_vk_to_scancode(WPARAM wparam) {
    if (wparam >= 'A' && wparam <= 'Z') return (SBGL_Scancode)(SBGL_SCANCODE_A + (wparam - 'A'));
    if (wparam >= '0' && wparam <= '9') return (SBGL_Scancode)(SBGL_SCANCODE_0 + (wparam - '0'));

    switch (wparam) {
        case VK_ESCAPE: return SBGL_SCANCODE_ESCAPE;
        case VK_RETURN: return SBGL_SCANCODE_RETURN;
        case VK_BACK:   return SBGL_SCANCODE_BACKSPACE;
        case VK_TAB:    return SBGL_SCANCODE_TAB;
        case VK_SPACE:  return SBGL_SCANCODE_SPACE;
        case VK_UP:     return SBGL_SCANCODE_UP;
        case VK_DOWN:   return SBGL_SCANCODE_DOWN;
        case VK_LEFT:   return SBGL_SCANCODE_LEFT;
        case VK_RIGHT:  return SBGL_SCANCODE_RIGHT;
        case VK_SHIFT:   return SBGL_SCANCODE_LSHIFT;
        case VK_CONTROL: return SBGL_SCANCODE_LCTRL;
        case VK_MENU:    return SBGL_SCANCODE_LALT;
        default:         return SBGL_SCANCODE_UNKNOWN;
    }
}

void win32_internal_process_message(UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN: {
            SBGL_Scancode code = win32_vk_to_scancode(wparam);
            if (code < SBGL_SCANCODE_MAX) g_keyboardState.keys[code] = true;
            break;
        }
        case WM_KEYUP:
        case WM_SYSKEYUP: {
            SBGL_Scancode code = win32_vk_to_scancode(wparam);
            if (code < SBGL_SCANCODE_MAX) g_keyboardState.keys[code] = false;
            break;
        }
        case WM_MOUSEMOVE: {
            g_mouseX = LOWORD(lparam);
            g_mouseY = HIWORD(lparam);
            break;
        }
        case WM_LBUTTONDOWN: g_mouseState[SBGL_MOUSE_BUTTON_LEFT] = true; break;
        case WM_LBUTTONUP:   g_mouseState[SBGL_MOUSE_BUTTON_LEFT] = false; break;
        case WM_RBUTTONDOWN: g_mouseState[SBGL_MOUSE_BUTTON_RIGHT] = true; break;
        case WM_RBUTTONUP:   g_mouseState[SBGL_MOUSE_BUTTON_RIGHT] = false; break;
        case WM_MBUTTONDOWN: g_mouseState[SBGL_MOUSE_BUTTON_MIDDLE] = true; break;
        case WM_MBUTTONUP:   g_mouseState[SBGL_MOUSE_BUTTON_MIDDLE] = false; break;
    }
}

void win32_internal_update_input_states(void) {
    g_prevKeyboardState = g_keyboardState;
    g_prevMouseX = g_mouseX;
    g_prevMouseY = g_mouseY;
}

bool sbgl_os_IsKeyDown(SBGL_Scancode key) { return (key < SBGL_SCANCODE_MAX) ? g_keyboardState.keys[key] : false; }
bool sbgl_os_IsKeyPressed(SBGL_Scancode key) { return (key < SBGL_SCANCODE_MAX) ? (g_keyboardState.keys[key] && !g_prevKeyboardState.keys[key]) : false; }
bool sbgl_os_IsMouseButtonDown(SBGL_MouseButton btn) { return (btn < SBGL_MOUSE_BUTTON_MAX) ? g_mouseState[btn] : false; }
void sbgl_os_GetMousePos(int* x, int* y) { *x = g_mouseX; *y = g_mouseY; }
void sbgl_os_GetMouseDelta(int* dx, int* dy) { *dx = g_mouseX - g_prevMouseX; *dy = g_mouseY - g_prevMouseY; }
