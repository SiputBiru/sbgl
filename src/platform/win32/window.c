#include "win32_internal.h"
#include "core/sbgl_platform.h"
#include "core/sbl_arena.h"
#include <stdio.h>
#include <stdlib.h>

HINSTANCE g_win32_instance = NULL;
HWND      g_win32_window   = NULL;

struct sbgl_Window {
    HWND hwnd;
    bool shouldClose;
    bool resized;
    int width, height;
    sbgl_InputState* input;
};

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    sbgl_Window* window = (sbgl_Window*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (msg) {
        case WM_CLOSE:
            if (window) window->shouldClose = true;
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_SIZE:
            if (window) {
                int w = LOWORD(lparam);
                int h = HIWORD(lparam);
                if (w > 0 && h > 0 && (w != window->width || h != window->height)) {
                    window->width = w;
                    window->height = h;
                    window->resized = true;
                }
            }
            break;
    }

    // Pass input messages to the input system
    if (window) win32_internal_process_message(window->input, msg, wparam, lparam);

    return DefWindowProc(hwnd, msg, wparam, lparam);
}

sbgl_Window* sbgl_os_CreateWindow(struct SblArena* arena, sbgl_InputState* input, int width, int height, const char* title) {
    if (!g_win32_instance) {
        g_win32_instance = GetModuleHandle(NULL);
    }

    WNDCLASSW wc = {0};
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = g_win32_instance;
    wc.lpszClassName = L"SBglWindowClass";
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);

    RegisterClassW(&wc);

    // Convert title to WideChar
    int title_len = MultiByteToWideChar(CP_UTF8, 0, title, -1, NULL, 0);
    wchar_t* wtitle = malloc(title_len * sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, title, -1, wtitle, title_len);

    HWND hwnd = CreateWindowExW(
        0, wc.lpszClassName, wtitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height,
        NULL, NULL, g_win32_instance, NULL
    );

    free(wtitle);

    if (!hwnd) return NULL;

    sbgl_Window* window = SBL_ARENA_PUSH_STRUCT(arena, sbgl_Window);
    window->hwnd = hwnd;
    window->shouldClose = false;
    window->resized = false;
    window->width = width;
    window->height = height;
    window->input = input;

    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)window);
    g_win32_window = hwnd;

    ShowWindow(hwnd, SW_SHOW);
    return window;
}

void sbgl_os_DestroyWindow(sbgl_Window* window) {
    if (!window) return;
    DestroyWindow(window->hwnd);
}

bool sbgl_os_WindowShouldClose(sbgl_Window* window) {
    return window ? window->shouldClose : true;
}

void sbgl_os_GetWindowSize(sbgl_Window* window, int* w, int* h) {
    if (window) {
        *w = window->width;
        *h = window->height;
    }
}

bool sbgl_os_WasWindowResized(sbgl_Window* window) {
    bool r = window->resized;
    window->resized = false;
    return r;
}


void sbgl_os_PollEvents(sbgl_Window* window) {
    if (window) win32_internal_update_input_states(window->input);

    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

uint64_t sbgl_os_GetPerfCount(void) {
    LARGE_INTEGER count;
    QueryPerformanceCounter(&count);
    return (uint64_t)count.QuadPart;
}

uint64_t sbgl_os_GetPerfFreq(void) {
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    return (uint64_t)freq.QuadPart;
}

void* sbgl_os_GetNativeWindowHandle(sbgl_Window* window) {
    return window ? (void*)window->hwnd : NULL;
}

void* sbgl_os_GetNativeInstanceHandle(void) {
    return (void*)g_win32_instance;
}

void* sbgl_os_GetNativeDisplayHandle(void) {
    return NULL; // Not used on Win32
}
