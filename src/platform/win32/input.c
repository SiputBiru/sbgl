#include "win32_internal.h"
#include <string.h>

static SBGL_Scancode win32_vk_to_scancode(WPARAM wparam) {
    if (wparam >= 'A' && wparam <= 'Z') return (SBGL_Scancode)(SBGL_SCANCODE_A + (wparam - 'A'));
    if (wparam == '0') return SBGL_SCANCODE_0;
    if (wparam >= '1' && wparam <= '9') return (SBGL_Scancode)(SBGL_SCANCODE_1 + (wparam - '1'));

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
        case 0xBB:       return SBGL_SCANCODE_EQUAL; // VK_OEM_PLUS
        case 0xBD:       return SBGL_SCANCODE_MINUS; // VK_OEM_MINUS
        default:         return SBGL_SCANCODE_UNKNOWN;
    }
}

void win32_internal_process_message(sbgl_InputState* input, UINT msg, WPARAM wparam, LPARAM lparam) {
    if (!input) return;
    switch (msg) {
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN: {
            SBGL_Scancode code = win32_vk_to_scancode(wparam);
            if (code < SBGL_SCANCODE_MAX) {
                if (!input->keysDown[code]) input->keysPressed[code] = true;
                input->keysDown[code] = true;
            }
            break;
        }
        case WM_KEYUP:
        case WM_SYSKEYUP: {
            SBGL_Scancode code = win32_vk_to_scancode(wparam);
            if (code < SBGL_SCANCODE_MAX) input->keysDown[code] = false;
            break;
        }
        case WM_MOUSEMOVE: {
            input->mouseX = LOWORD(lparam);
            input->mouseY = HIWORD(lparam);
            break;
        }
        case WM_LBUTTONDOWN: input->mouseDown[SBGL_MOUSE_BUTTON_LEFT] = true; break;
        case WM_LBUTTONUP:   input->mouseDown[SBGL_MOUSE_BUTTON_LEFT] = false; break;
        case WM_RBUTTONDOWN: input->mouseDown[SBGL_MOUSE_BUTTON_RIGHT] = true; break;
        case WM_RBUTTONUP:   input->mouseDown[SBGL_MOUSE_BUTTON_RIGHT] = false; break;
        case WM_MBUTTONDOWN: input->mouseDown[SBGL_MOUSE_BUTTON_MIDDLE] = true; break;
        case WM_MBUTTONUP:   input->mouseDown[SBGL_MOUSE_BUTTON_MIDDLE] = false; break;
        case WM_KILLFOCUS: {
            /* Input states are cleared when the window loses focus to prevent 
               stuck keys or buttons from affecting the simulation. */
            memset(input->keysDown, 0, sizeof(input->keysDown));
            memset(input->mouseDown, 0, sizeof(input->mouseDown));
            break;
        }
    }
}

void win32_internal_update_input_states(sbgl_InputState* input) {
    if (!input) return;
    input->mouseDeltaX = input->mouseX - input->_internalMouseX;
    input->mouseDeltaY = input->mouseY - input->_internalMouseY;
    input->_internalMouseX = input->mouseX;
    input->_internalMouseY = input->mouseY;
}
