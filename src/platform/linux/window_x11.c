#include "linux_internal.h"
#include "core/sbgl_platform.h"
#include "core/sbl_arena.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/extensions/Xfixes.h>
#include <time.h>
#include <string.h>

// Defined in input_x11.c
void x11_internal_process_event(XEvent* event, sbgl_Window* window);

sbgl_Window* sbgl_os_CreateWindow(struct SblArena* arena, sbgl_InputState* input, int width, int height, const char* title) {
    Display* display = XOpenDisplay(NULL);
    if (!display) return NULL;

    int screen = DefaultScreen(display);
    
    Window win = XCreateSimpleWindow(
        display, RootWindow(display, screen),
        0, 0, (unsigned int)width, (unsigned int)height, 0,
        BlackPixel(display, screen),
        WhitePixel(display, screen)
    );

    XStoreName(display, win, title);
    XSelectInput(display, win, ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | StructureNotifyMask | PointerMotionMask | FocusChangeMask);

    Atom wmDeleteMessage = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, win, &wmDeleteMessage, 1);

    XMapWindow(display, win);
    XFlush(display);

    sbgl_Window* window = SBL_ARENA_PUSH_STRUCT_ZERO(arena, sbgl_Window);
    if (!window) {
        XCloseDisplay(display);
        return NULL;
    }
    window->display = display;
    window->window = win;
    window->wmDeleteMessage = wmDeleteMessage;
    window->input = input;
    window->shouldClose = false;
    window->resized = false;
    window->width = width;
    window->height = height;

    return window;
}

void sbgl_os_DestroyWindow(sbgl_Window* window) {
    if (!window) return;
    XDestroyWindow(window->display, window->window);
    XCloseDisplay(window->display);
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

bool sbgl_os_IsWindowFocused(sbgl_Window* window) {
    return window ? window->focused : false;
}

void sbgl_os_SetCursorVisible(sbgl_Window* window, bool visible) {
    if (!window || !window->display) return;
    
    // Adjusts cursor visibility using XFixes extension.
    if (visible) {
        XFixesShowCursor(window->display, window->window);
    } else {
        XFixesHideCursor(window->display, window->window);
    }
    XFlush(window->display);
}

void sbgl_os_SetCursorLocked(sbgl_Window* window, bool locked) {
    if (!window || !window->display) return;

    // Constrains or releases the pointer relative to the window.
    if (locked) {
        XGrabPointer(window->display, window->window, False,
                     ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
                     GrabModeAsync, GrabModeAsync,
                     window->window, None, CurrentTime);
    } else {
        XUngrabPointer(window->display, CurrentTime);
    }
    XFlush(window->display);
}

void sbgl_os_PollEvents(sbgl_Window* window) {
    if (!window || !window->display) return;

    linux_internal_update_input_states(window);

    while (XPending(window->display)) {
        XEvent event;
        XNextEvent(window->display, &event);

        if (event.type == ClientMessage) {
            if ((Atom)event.xclient.data.l[0] == window->wmDeleteMessage) {
                window->shouldClose = true;
            }
        } else if (event.type == ConfigureNotify) {
            int w = event.xconfigure.width;
            int h = event.xconfigure.height;
            if (w > 0 && h > 0 && (w != window->width || h != window->height)) {
                window->width = w;
                window->height = h;
                window->resized = true;
            }
        }

        // Pass event to input system
        x11_internal_process_event(&event, window);
    }
}


uint64_t sbgl_os_GetPerfCount(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

uint64_t sbgl_os_GetPerfFreq(void) {
    return 1000000000ULL;
}

void* sbgl_os_GetNativeWindowHandle(sbgl_Window* window) {
    return window ? (void*)(uintptr_t)window->window : NULL;
}

void* sbgl_os_GetNativeInstanceHandle(sbgl_Window* window) {
    (void)window;
    return NULL;
}

void* sbgl_os_GetNativeDisplayHandle(sbgl_Window* window) {
    return window ? (void*)window->display : NULL;
}
