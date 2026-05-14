#include "core/sbgl_platform.h"
#include "core/sbl_arena.h"
#include "win32_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Reports errors to both debugger output and stderr.
 * 
 * Uses OutputDebugStringW for Visual Studio debugger visibility
 * and fprintf(stderr, ...) for console output.
 */
static void win32_report_error(const wchar_t* message) {
    // Output to debugger (visible in Visual Studio Output window)
    OutputDebugStringW(message);
    OutputDebugStringW(L"\n");
    
    // Output to stderr (visible in console)
    int len = WideCharToMultiByte(CP_UTF8, 0, message, -1, NULL, 0, NULL, NULL);
    if (len > 0) {
        char* utf8 = (char*)malloc(len);
        if (utf8) {
            WideCharToMultiByte(CP_UTF8, 0, message, -1, utf8, len, NULL, NULL);
            fprintf(stderr, "[Win32] %s\n", utf8);
            free(utf8);
        }
    }
}

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
    case WM_INPUT: {
        // Handle raw input for high-precision mouse deltas when cursor is locked
        if (window && window->cursorLocked) {
            RAWINPUT raw;
            UINT size = sizeof(raw);
            
            if (GetRawInputData((HRAWINPUT)lparam, RID_INPUT, &raw, 
                                &size, sizeof(RAWINPUTHEADER)) != (UINT)-1) {
                if (raw.header.dwType == RIM_TYPEMOUSE) {
                    // Accumulate raw deltas
                    window->accumulatedDeltaX += raw.mouse.lLastX;
                    window->accumulatedDeltaY += raw.mouse.lLastY;
                }
            }
        }
        break;
    }
    case WM_DPICHANGED: {
        // Handle DPI changes for high-DPI display support
        if (window) {
            RECT* const prcNewWindow = (RECT*)lparam;
            SetWindowPos(window->hwnd, NULL,
                prcNewWindow->left,
                prcNewWindow->top,
                prcNewWindow->right - prcNewWindow->left,
                prcNewWindow->bottom - prcNewWindow->top,
                SWP_NOZORDER | SWP_NOACTIVATE);
            window->resized = true;
        }
        break;
    }
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
    // Set DPI awareness for Windows 10+ (Per-Monitor V2)
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    
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

    // Calculate proper window size for requested client area
    RECT rect = { 0, 0, width, height };
    AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, FALSE, 0);

    HWND hwnd = CreateWindowExW(
        0,
        wc.lpszClassName,
        wtitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        rect.right - rect.left,     // Adjusted width including borders
        rect.bottom - rect.top,     // Adjusted height including title bar
        NULL,
        NULL,
        hinstance,
        NULL
    );

    free(wtitle);

    if (!hwnd) {
        win32_report_error(L"Failed to create window: CreateWindowExW returned NULL");
        return NULL;
    }

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
    window->cursorLocked = false;
    window->input = input;
    window->accumulatedDeltaX = 0;
    window->accumulatedDeltaY = 0;
    
    /* Store class name for unregistration on destroy */
    wcsncpy(window->className, wc.lpszClassName, 255);
    window->className[255] = L'\0';

    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)window);

    ShowWindow(hwnd, SW_SHOW);
    return window;
}

void sbgl_os_DestroyWindow(sbgl_Window* window) {
    if (!window)
        return;
    
    /* Unregister raw input if cursor is still locked */
    if (window->cursorLocked) {
        RAWINPUTDEVICE rid = {
            .usUsagePage = 0x01,
            .usUsage = 0x02,
            .dwFlags = RIDEV_REMOVE,
            .hwndTarget = NULL
        };
        RegisterRawInputDevices(&rid, 1, sizeof(rid));
        ClipCursor(NULL);
    }
    
    DestroyWindow(window->hwnd);
    
    /* Unregister window class to prevent resource leaks */
    if (window->className[0] != L'\0') {
        UnregisterClassW(window->className, window->hinstance);
    }
}

bool sbgl_os_WindowShouldClose(sbgl_Window* window) { return window ? window->shouldClose : true; }

void sbgl_os_GetWindowSize(sbgl_Window* window, int* w, int* h) {
    if (!window) {
        if (w) *w = 0;
        if (h) *h = 0;
        return;
    }
    if (w) *w = window->width;
    if (h) *h = window->height;
}

bool sbgl_os_WasWindowResized(sbgl_Window* window) {
    if (!window)
        return false;
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
    if (!window || window->cursorLocked == locked)
        return;

    window->cursorLocked = locked;

    if (locked) {
        /* Register for raw mouse input to get high-precision deltas */
        RAWINPUTDEVICE rid = {
            .usUsagePage = 0x01,    // Generic Desktop
            .usUsage = 0x02,        // Mouse
            .dwFlags = RIDEV_NOLEGACY | RIDEV_CAPTUREMOUSE,
            .hwndTarget = window->hwnd
        };
        if (!RegisterRawInputDevices(&rid, 1, sizeof(rid))) {
            win32_report_error(L"Failed to register raw input device");
        }

        /* The cursor is confined to the window client area by clipping it 
           to the screen-space coordinates of the client rectangle. */
        RECT rect;
        GetClientRect(window->hwnd, &rect);
        ClientToScreen(window->hwnd, (POINT*)&rect.left);
        ClientToScreen(window->hwnd, (POINT*)&rect.right);
        ClipCursor(&rect);

        /* Hide the cursor */
        ShowCursor(FALSE);
        window->cursorVisible = false;

        /* The cursor is initially centered within the window to ensure 
           consistent relative motion tracking. */
        int centerX = window->width / 2;
        int centerY = window->height / 2;
        POINT p = { centerX, centerY };
        ClientToScreen(window->hwnd, &p);
        SetCursorPos(p.x, p.y);
    } else {
        /* Unregister raw input device */
        RAWINPUTDEVICE rid = {
            .usUsagePage = 0x01,
            .usUsage = 0x02,
            .dwFlags = RIDEV_REMOVE,
            .hwndTarget = NULL
        };
        RegisterRawInputDevices(&rid, 1, sizeof(rid));

        /* Any existing cursor clipping is removed, allowing the pointer 
           to move freely across the entire desktop. */
        ClipCursor(NULL);

        /* Show the cursor */
        ShowCursor(TRUE);
        window->cursorVisible = true;
    }
}

bool sbgl_os_IsWindowFocused(sbgl_Window* window) {
    /* Returns the current focus state as tracked by window messages. */
    return window ? window->focused : false;
}

void sbgl_os_PollEvents(sbgl_Window* window) {
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    // Update input states after processing all messages
    if (window)
        win32_internal_update_input_states(window->input, window);
}

uint64_t sbgl_os_GetPerfCount(sbgl_Window* window) {
    (void)window; // Not used, but kept for API consistency
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
