#include "core/sbgl_platform.h"
#include "core/sbl_arena.h"
#include "win32_internal.h"
#include <stdio.h>
#include <stdlib.h>

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    sbgl_Window* window = (sbgl_Window*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (msg) {
    case WM_CLOSE:
        if (window)
            window->shouldClose = true;
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
    case WM_SETFOCUS:
        if (window)
            window->focused = true;
        break;
    case WM_KILLFOCUS:
        if (window)
            window->focused = false;
        break;
    }

    // Pass input messages to the input system
    if (window)
        win32_internal_process_message(window->input, msg, wparam, lparam);

    return DefWindowProc(hwnd, msg, wparam, lparam);
}

sbgl_Window* sbgl_os_CreateWindow(
    struct SblArena* arena,
    sbgl_InputState* input,
    int width,
    int height,
    const char* title
) {
    HINSTANCE hinstance = GetModuleHandle(NULL);

    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hinstance;
    wc.lpszClassName = L"SBglWindowClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClassW(&wc);

    // Convert title to WideChar
    int title_len = MultiByteToWideChar(CP_UTF8, 0, title, -1, NULL, 0);
    wchar_t* wtitle = malloc(title_len * sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, title, -1, wtitle, title_len);

    HWND hwnd = CreateWindowExW(
        0,
        wc.lpszClassName,
        wtitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        width,
        height,
        NULL,
        NULL,
        hinstance,
        NULL
    );

    free(wtitle);

    if (!hwnd)
        return NULL;

    sbgl_Window* window = SBL_ARENA_PUSH_STRUCT_ZERO(arena, sbgl_Window);
    if (!window) {
        DestroyWindow(hwnd);
        return NULL;
    }
    window->hinstance = hinstance;
    window->hwnd = hwnd;
    window->shouldClose = false;
    window->resized = false;
    window->width = width;
    window->height = height;
    window->focused = true;
    window->cursorVisible = true;
    window->input = input;

    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)window);

    ShowWindow(hwnd, SW_SHOW);
    return window;
}

void sbgl_os_DestroyWindow(sbgl_Window* window) {
    if (!window)
        return;
    DestroyWindow(window->hwnd);
}

bool sbgl_os_WindowShouldClose(sbgl_Window* window) { return window ? window->shouldClose : true; }

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

void sbgl_os_SetCursorVisible(sbgl_Window* window, bool visible) {
    if (!window || window->cursorVisible == visible)
        return;

    /* The cursor visibility state is updated and the system cursor counter 
       is incremented or decremented accordingly. */
    window->cursorVisible = visible;
    if (visible) {
        ShowCursor(TRUE);
    } else {
        ShowCursor(FALSE);
    }
}

void sbgl_os_SetCursorLocked(sbgl_Window* window, bool locked) {
    if (!window)
        return;

    if (locked) {
        /* The cursor is confined to the window client area by clipping it 
           to the screen-space coordinates of the client rectangle. */
        RECT rect;
        GetClientRect(window->hwnd, &rect);
        ClientToScreen(window->hwnd, (POINT*)&rect.left);
        ClientToScreen(window->hwnd, (POINT*)&rect.right);
        ClipCursor(&rect);

        /* The cursor is initially centered within the window to ensure 
           consistent relative motion tracking. */
        int centerX = window->width / 2;
        int centerY = window->height / 2;
        POINT p = { centerX, centerY };
        ClientToScreen(window->hwnd, &p);
        SetCursorPos(p.x, p.y);
    } else {
        /* Any existing cursor clipping is removed, allowing the pointer 
           to move freely across the entire desktop. */
        ClipCursor(NULL);
    }
}

bool sbgl_os_IsWindowFocused(sbgl_Window* window) {
    /* Returns the current focus state as tracked by window messages. */
    return window ? window->focused : false;
}

void sbgl_os_PollEvents(sbgl_Window* window) {
    if (window)
        win32_internal_update_input_states(window->input);

    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

uint64_t sbgl_os_GetPerfCount(sbgl_Window* window) {
    (void)window;
    LARGE_INTEGER count;
    QueryPerformanceCounter(&count);
    return (uint64_t)count.QuadPart;
}

uint64_t sbgl_os_GetPerfFreq(sbgl_Window* window) {
    (void)window;
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    return (uint64_t)freq.QuadPart;
}

void* sbgl_os_GetNativeWindowHandle(sbgl_Window* window) {
    return window ? (void*)window->hwnd : NULL;
}

void* sbgl_os_GetNativeInstanceHandle(sbgl_Window* window) {
    return window ? (void*)window->hinstance : NULL;
}

void* sbgl_os_GetNativeDisplayHandle(sbgl_Window* window) {
    (void)window;
    return NULL; // Not used on Win32
}
