#ifndef LINUX_INTERNAL_H
#define LINUX_INTERNAL_H

#include "sbgl_types.h"
#include "core/sbgl_input.h"
#include <stdint.h>

#ifdef SBGL_PLATFORM_WAYLAND
#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"

// Shared Wayland state
extern struct wl_display*    g_display;
extern struct wl_compositor* g_compositor;
extern struct xdg_wm_base*   g_wm_base;
extern struct wl_seat*       g_seat;

void linux_init_input(struct wl_registry* registry, uint32_t name, uint32_t version);
#else
#include <X11/Xlib.h>

// Shared X11 state
extern Display* g_x11_display;

#endif

// Internal update function called by PollEvents
void linux_internal_update_input_states(sbgl_InputState* input);

#endif // LINUX_INTERNAL_H
