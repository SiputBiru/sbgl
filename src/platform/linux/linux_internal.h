#ifndef LINUX_INTERNAL_H
#define LINUX_INTERNAL_H

#include "sbgl_types.h"
#include "core/sbgl_input.h"
#include <stdint.h>

#include <stdbool.h>

#ifdef SBGL_PLATFORM_WAYLAND
#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"
#include "pointer-constraints-unstable-v1-client-protocol.h"
#include "relative-pointer-unstable-v1-client-protocol.h"

/**
 * @brief Native Wayland window state.
 */
struct sbgl_Window {
    struct wl_display*    display;
    struct wl_compositor* compositor;
    struct xdg_wm_base*   wm_base;
    struct wl_seat*       seat;
    struct wl_surface*    surface;
    struct xdg_surface*   xdg_surface;
    struct xdg_toplevel*  xdg_toplevel;
    struct zwp_pointer_constraints_v1* pointer_constraints;
    struct zwp_locked_pointer_v1* locked_pointer;
    struct zwp_relative_pointer_manager_v1* relative_pointer_manager;
    struct zwp_relative_pointer_v1* relative_pointer;
    sbgl_InputState*      input;
    uint32_t              pointer_serial;
    bool shouldClose;
    bool resized;
    bool focused;
    bool cursor_visible;
    int width, height;
};

void linux_init_input(struct wl_registry* registry, uint32_t name, uint32_t version, sbgl_Window* window);

extern const struct wl_keyboard_listener keyboard_listener;
extern const struct wl_pointer_listener pointer_listener;
#else
#include <X11/Xlib.h>

/**
 * @brief Native X11 window state.
 */
struct sbgl_Window {
    Display* display;
    Window window;
    Atom wmDeleteMessage;
    sbgl_InputState* input;
    bool shouldClose;
    bool resized;
    bool focused;
    int width, height;
};

void x11_internal_process_event(XEvent* event, struct sbgl_Window* window);
#endif

// Internal update function called by PollEvents
void linux_internal_update_input_states(struct sbgl_Window* window);

#endif // LINUX_INTERNAL_H
