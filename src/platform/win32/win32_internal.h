#ifndef WIN32_INTERNAL_H
#define WIN32_INTERNAL_H

#ifndef UNICODE
#define UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "core/sbgl_input.h"

// Shared Win32 state
extern HINSTANCE g_win32_instance;
extern HWND      g_win32_window;

// Internal event processing
void win32_internal_process_message(UINT msg, WPARAM wparam, LPARAM lparam);
void win32_internal_update_input_states(void);

#endif // WIN32_INTERNAL_H
