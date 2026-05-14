#include "win32_internal.h"
#include <string.h>

static SBGL_Scancode win32_vk_to_scancode(WPARAM wparam) {
    if (wparam >= 'A' && wparam <= 'Z') return (SBGL_Scancode)(SBGL_SCANCODE_A + (wparam - 'A'));
    if (wparam == '0') return SBGL_SCANCODE_0;
    if (wparam >= '1' && wparam <= '9') return (SBGL_Scancode)(SBGL_SCANCODE_1 + (wparam - '1'));

    switch (wparam) {
        /* Basic keys */
        case VK_ESCAPE: return SBGL_SCANCODE_ESCAPE;
        case VK_RETURN: return SBGL_SCANCODE_RETURN;
        case VK_BACK:   return SBGL_SCANCODE_BACKSPACE;
        case VK_TAB:    return SBGL_SCANCODE_TAB;
        case VK_SPACE:  return SBGL_SCANCODE_SPACE;
        case VK_UP:     return SBGL_SCANCODE_UP;
        case VK_DOWN:   return SBGL_SCANCODE_DOWN;
        case VK_LEFT:   return SBGL_SCANCODE_LEFT;
        case VK_RIGHT:  return SBGL_SCANCODE_RIGHT;
        
        /* Left modifiers */
        case VK_LSHIFT:   return SBGL_SCANCODE_LSHIFT;
        case VK_LCONTROL: return SBGL_SCANCODE_LCTRL;
        case VK_LMENU:    return SBGL_SCANCODE_LALT;
        
        /* Right modifiers */
        case VK_RSHIFT:   return SBGL_SCANCODE_RSHIFT;
        case VK_RCONTROL: return SBGL_SCANCODE_RCTRL;
        case VK_RMENU:    return SBGL_SCANCODE_RALT;
        
        /* Legacy modifier fallbacks */
        case VK_SHIFT:   return SBGL_SCANCODE_LSHIFT;
        case VK_CONTROL: return SBGL_SCANCODE_LCTRL;
        case VK_MENU:    return SBGL_SCANCODE_LALT;
        
        /* Symbol keys */
        case VK_OEM_PLUS:     return SBGL_SCANCODE_EQUAL;
        case VK_OEM_MINUS:    return SBGL_SCANCODE_MINUS;
        case VK_OEM_1:        return SBGL_SCANCODE_SEMICOLON;    // ;:
        case VK_OEM_2:        return SBGL_SCANCODE_SLASH;        // /?
        case VK_OEM_3:        return SBGL_SCANCODE_GRAVE;        // `~
        case VK_OEM_4:        return SBGL_SCANCODE_LEFTBRACKET;  // [{
        case VK_OEM_5:        return SBGL_SCANCODE_BACKSLASH;    // \|
        case VK_OEM_6:        return SBGL_SCANCODE_RIGHTBRACKET; // ]}
        case VK_OEM_7:        return SBGL_SCANCODE_APOSTROPHE;   // '"
        case VK_OEM_COMMA:    return SBGL_SCANCODE_COMMA;        // ,<
        case VK_OEM_PERIOD:   return SBGL_SCANCODE_PERIOD;       // .>
        
        /* Function keys */
        case VK_F1:  return SBGL_SCANCODE_F1;
        case VK_F2:  return SBGL_SCANCODE_F2;
        case VK_F3:  return SBGL_SCANCODE_F3;
        case VK_F4:  return SBGL_SCANCODE_F4;
        case VK_F5:  return SBGL_SCANCODE_F5;
        case VK_F6:  return SBGL_SCANCODE_F6;
        case VK_F7:  return SBGL_SCANCODE_F7;
        case VK_F8:  return SBGL_SCANCODE_F8;
        case VK_F9:  return SBGL_SCANCODE_F9;
        case VK_F10: return SBGL_SCANCODE_F10;
        case VK_F11: return SBGL_SCANCODE_F11;
        case VK_F12: return SBGL_SCANCODE_F12;
        
        /* Navigation keys */
        case VK_INSERT:    return SBGL_SCANCODE_INSERT;
        case VK_DELETE:    return SBGL_SCANCODE_DELETE;
        case VK_HOME:      return SBGL_SCANCODE_HOME;
        case VK_END:       return SBGL_SCANCODE_END;
        case VK_PRIOR:     return SBGL_SCANCODE_PAGEUP;
        case VK_NEXT:      return SBGL_SCANCODE_PAGEDOWN;
        
        /* Numpad */
        case VK_NUMPAD0:   return SBGL_SCANCODE_KP_0;
        case VK_NUMPAD1:   return SBGL_SCANCODE_KP_1;
        case VK_NUMPAD2:   return SBGL_SCANCODE_KP_2;
        case VK_NUMPAD3:   return SBGL_SCANCODE_KP_3;
        case VK_NUMPAD4:   return SBGL_SCANCODE_KP_4;
        case VK_NUMPAD5:   return SBGL_SCANCODE_KP_5;
        case VK_NUMPAD6:   return SBGL_SCANCODE_KP_6;
        case VK_NUMPAD7:   return SBGL_SCANCODE_KP_7;
        case VK_NUMPAD8:   return SBGL_SCANCODE_KP_8;
        case VK_NUMPAD9:   return SBGL_SCANCODE_KP_9;
        case VK_MULTIPLY:  return SBGL_SCANCODE_KP_MULTIPLY;
        case VK_ADD:       return SBGL_SCANCODE_KP_PLUS;
        case VK_SUBTRACT:  return SBGL_SCANCODE_KP_MINUS;
        case VK_DECIMAL:   return SBGL_SCANCODE_KP_PERIOD;
        case VK_DIVIDE:    return SBGL_SCANCODE_KP_DIVIDE;
        case VK_SEPARATOR: return SBGL_SCANCODE_KP_ENTER;
        
        /* Lock keys */
        case VK_CAPITAL:   return SBGL_SCANCODE_CAPSLOCK;
        case VK_NUMLOCK:   return SBGL_SCANCODE_NUMLOCKCLEAR;
        case VK_SCROLL:    return SBGL_SCANCODE_SCROLLLOCK;
        
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
                // Filter auto-repeat: bit 30 of lparam indicates previous key state
                // If set, this is an auto-repeat message, not an initial press
                bool isRepeat = (lparam & (1 << 30)) != 0;
                if (!isRepeat) {
                    input->keysPressed[code] = true;
                }
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

void win32_internal_update_input_states(sbgl_InputState* input, sbgl_Window* window) {
    if (!input) return;
    
    if (window && window->cursorLocked) {
        // Use accumulated raw input deltas for high-precision camera control
        input->mouseDeltaX = window->accumulatedDeltaX;
        input->mouseDeltaY = window->accumulatedDeltaY;
        
        // Reset accumulators for next frame
        window->accumulatedDeltaX = 0;
        window->accumulatedDeltaY = 0;
        
        // Keep internal tracking in sync
        input->_internalMouseX = input->mouseX;
        input->_internalMouseY = input->mouseY;
    } else {
        // Use standard position-based delta calculation
        input->mouseDeltaX = input->mouseX - input->_internalMouseX;
        input->mouseDeltaY = input->mouseY - input->_internalMouseY;
        input->_internalMouseX = input->mouseX;
        input->_internalMouseY = input->mouseY;
    }
}
