#include "linux_internal.h"
#include "core/sbgl_platform.h"
#include "core/sbl_arena.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

Display* g_x11_display = NULL;

struct sbgl_Window {
    Window window;
    Atom wmDeleteMessage;
    bool shouldClose;
    int width, height;
};

// Defined in input_x11.c
void x11_internal_process_event(XEvent* event, sbgl_Window* window);

sbgl_Window* sbgl_os_CreateWindow(struct SblArena* arena, int width, int height, const char* title) {
    if (!g_x11_display) {
        g_x11_display = XOpenDisplay(NULL);
        if (!g_x11_display) return NULL;
    }

    int screen = DefaultScreen(g_x11_display);
    
    Window win = XCreateSimpleWindow(
        g_x11_display, RootWindow(g_x11_display, screen),
        0, 0, (unsigned int)width, (unsigned int)height, 0,
        BlackPixel(g_x11_display, screen),
        WhitePixel(g_x11_display, screen)
    );

    XStoreName(g_x11_display, win, title);
    XSelectInput(g_x11_display, win, ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | StructureNotifyMask | PointerMotionMask);

    Atom wmDeleteMessage = XInternAtom(g_x11_display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(g_x11_display, win, &wmDeleteMessage, 1);

    XMapWindow(g_x11_display, win);
    XFlush(g_x11_display);

    sbgl_Window* window = SBL_ARENA_PUSH_STRUCT(arena, sbgl_Window);
    window->window = win;
    window->wmDeleteMessage = wmDeleteMessage;
    window->shouldClose = false;
    window->width = width;
    window->height = height;

    return window;
}

void sbgl_os_DestroyWindow(sbgl_Window* window) {
    if (!window) return;
    XDestroyWindow(g_x11_display, window->window);
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

void sbgl_os_PollEvents(sbgl_Window* window) {
    if (!g_x11_display || !window) return;

    linux_internal_update_input_states();

    while (XPending(g_x11_display)) {
        XEvent event;
        XNextEvent(g_x11_display, &event);

        if (event.type == ClientMessage) {
            if ((Atom)event.xclient.data.l[0] == window->wmDeleteMessage) {
                window->shouldClose = true;
            }
        } else if (event.type == ConfigureNotify) {
            window->width = event.xconfigure.width;
            window->height = event.xconfigure.height;
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

void* sbgl_os_GetNativeInstanceHandle(void) {
    return NULL;
}

void* sbgl_os_GetNativeDisplayHandle(void) {
    return (void*)g_x11_display;
}
