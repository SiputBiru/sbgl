#ifndef LINUX_INTERNAL_H
#define LINUX_INTERNAL_H

#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"
#include "core/sbgl_input.h"

// Shared Linux/Wayland state
extern struct wl_display*    g_display;
extern struct wl_compositor* g_compositor;
extern struct xdg_wm_base*   g_wm_base;
extern struct wl_seat*       g_seat;

// Internal Registry Handlers
void linux_init_input(struct wl_registry* registry, uint32_t name, uint32_t version);

#endif // LINUX_INTERNAL_H
