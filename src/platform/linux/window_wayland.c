#include "linux_internal.h"
#include "core/sbgl_platform.h"
#include "core/sbl_arena.h"
#include <stdio.h>
#include <time.h>
#include <string.h>

struct wl_display*    g_display = NULL;
struct wl_compositor* g_compositor = NULL;
struct xdg_wm_base*   g_wm_base = NULL;
struct wl_seat*       g_seat = NULL;

/**
 * @brief Native Wayland window state.
 * 
 * Wraps the Wayland protocols required for windowing and resizing.
 */
struct sbgl_Window {
    struct wl_surface* surface;     /**< The raw Wayland drawing surface. */
    struct xdg_surface* xdg_surface; /**< The XDG-Shell surface wrapper. */
    struct xdg_toplevel* xdg_toplevel; /**< The top-level window handle. */
    sbgl_InputState*     input;     /**< Pointer to the context's input state. */
    bool shouldClose;               /**< Flag set when the OS requests closure. */
    bool resized;                   /**< Flag set when a resize event occurs. */
    int width, height;               /**< Current client dimensions. */
};

// Internal update function from input.c
void linux_internal_update_input_states(sbgl_InputState* input);

// Exported from input_wayland.c
extern const struct wl_keyboard_listener keyboard_listener;
extern const struct wl_pointer_listener pointer_listener;

// --- Window Manager Listeners ---
static void wm_base_ping(void* data, struct xdg_wm_base* wm_base, uint32_t serial) {
    xdg_wm_base_pong(wm_base, serial);
}
static const struct xdg_wm_base_listener wm_base_listener = { .ping = wm_base_ping };

// --- Registry ---
static void registry_handle_global(void* data, struct wl_registry* registry, uint32_t name, const char* interface, uint32_t version) {
    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        g_compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 1);
    } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
        g_wm_base = wl_registry_bind(registry, name, &xdg_wm_base_interface, 1);
        xdg_wm_base_add_listener(g_wm_base, &wm_base_listener, NULL);
    } else if (strcmp(interface, wl_seat_interface.name) == 0) {
        linux_init_input(registry, name, 1);
    }
}
static const struct wl_registry_listener registry_listener = { .global = registry_handle_global };

// --- Window Listeners ---
static void toplevel_handle_configure(void* data, struct xdg_toplevel* t, int32_t w, int32_t h, struct wl_array* s) {
    sbgl_Window* win = (sbgl_Window*)data;
    if (w > 0 && h > 0 && (w != win->width || h != win->height)) {
        win->width = w;
        win->height = h;
        win->resized = true;
    }
}
static void toplevel_handle_close(void* data, struct xdg_toplevel* t) {
    ((sbgl_Window*)data)->shouldClose = true;
}
static const struct xdg_toplevel_listener toplevel_listener = { .configure = toplevel_handle_configure, .close = toplevel_handle_close };

static void xdg_surface_handle_configure(void* data, struct xdg_surface* surf, uint32_t serial) {
    xdg_surface_ack_configure(surf, serial);
}
static const struct xdg_surface_listener xdg_surface_listener = { .configure = xdg_surface_handle_configure };

// --- HAL ---
sbgl_Window* sbgl_os_CreateWindow(struct SblArena* arena, sbgl_InputState* input, int width, int height, const char* title) {
    if (!g_display) {
        g_display = wl_display_connect(NULL);
        struct wl_registry* reg = wl_display_get_registry(g_display);
        wl_registry_add_listener(reg, &registry_listener, NULL);
        wl_display_roundtrip(g_display);
    }
    sbgl_Window* window = SBL_ARENA_PUSH_STRUCT(arena, sbgl_Window);
    window->width = width; window->height = height;
    window->shouldClose = false;
    window->resized = false;
    window->input = input;

    window->surface = wl_compositor_create_surface(g_compositor);
    window->xdg_surface = xdg_wm_base_get_xdg_surface(g_wm_base, window->surface);
    xdg_surface_add_listener(window->xdg_surface, &xdg_surface_listener, window);
    window->xdg_toplevel = xdg_surface_get_toplevel(window->xdg_surface);
    xdg_toplevel_add_listener(window->xdg_toplevel, &toplevel_listener, window);
    xdg_toplevel_set_title(window->xdg_toplevel, title);

    if (g_seat) {
        struct wl_keyboard* k = wl_seat_get_keyboard(g_seat);
        if (k) wl_keyboard_add_listener(k, &keyboard_listener, window);
        struct wl_pointer* p = wl_seat_get_pointer(g_seat);
        if (p) wl_pointer_add_listener(p, &pointer_listener, window);
    }

    wl_surface_commit(window->surface);
    wl_display_roundtrip(g_display);
    return window;
}

void sbgl_os_PollEvents(sbgl_Window* window) {
    linux_internal_update_input_states(window->input);
    while (wl_display_prepare_read(g_display) != 0) wl_display_dispatch_pending(g_display);
    wl_display_flush(g_display);
    wl_display_read_events(g_display);
    wl_display_dispatch_pending(g_display);
}

bool sbgl_os_WindowShouldClose(sbgl_Window* window) { return window->shouldClose; }
void sbgl_os_GetWindowSize(sbgl_Window* window, int* w, int* h) { *w = window->width; *h = window->height; }

bool sbgl_os_WasWindowResized(sbgl_Window* window) {
    bool r = window->resized;
    window->resized = false;
    return r;
}

uint64_t sbgl_os_GetPerfCount(void) { struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec; }
uint64_t sbgl_os_GetPerfFreq(void) { return 1000000000ULL; }
void* sbgl_os_GetNativeWindowHandle(sbgl_Window* window) { return (void*)window->surface; }
void* sbgl_os_GetNativeDisplayHandle(void) { return (void*)g_display; }
void sbgl_os_DestroyWindow(sbgl_Window* window) { xdg_toplevel_destroy(window->xdg_toplevel); xdg_surface_destroy(window->xdg_surface); wl_surface_destroy(window->surface); }
