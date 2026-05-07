#ifndef WIN32_INTERNAL_H
#define WIN32_INTERNAL_H

#ifndef UNICODE
#define UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#include "core/sbgl_input.h"
#include "sbgl_types.h"
#include <windows.h>

/**
 * @brief Native Win32 window state.
 */
struct sbgl_Window {
    HINSTANCE hinstance;
    HWND hwnd;
    bool shouldClose;
    bool resized;
    bool focused;
    bool cursorVisible;
    int width, height;
    sbgl_InputState* input;
};

// Internal event processing
void win32_internal_process_message(sbgl_InputState* input, UINT msg, WPARAM wparam, LPARAM lparam);
void win32_internal_update_input_states(sbgl_InputState* input);

#endif // WIN32_INTERNAL_H
