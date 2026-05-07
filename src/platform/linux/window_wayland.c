#define _POSIX_C_SOURCE 199309L
#include "linux_internal.h"
#include "core/sbgl_platform.h"
#include "core/sbl_arena.h"
#include <string.h>
#include <time.h>


// --- Window Manager Listeners ---
static void wm_base_ping(void* data, struct xdg_wm_base* wm_base, uint32_t serial) {
    (void)data;
    xdg_wm_base_pong(wm_base, serial);
}
static const struct xdg_wm_base_listener wm_base_listener = { .ping = wm_base_ping };

// --- Registry ---
static void registry_handle_global(void* data, struct wl_registry* registry, uint32_t name, const char* interface, uint32_t version) {
    (void)version;
    sbgl_Window* window = (sbgl_Window*)data;
    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        window->compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 1);
    } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
        window->wm_base = wl_registry_bind(registry, name, &xdg_wm_base_interface, 1);
        xdg_wm_base_add_listener(window->wm_base, &wm_base_listener, NULL);
    } else if (strcmp(interface, wl_seat_interface.name) == 0) {
        linux_init_input(registry, name, 1, window);
    } else if (strcmp(interface, zwp_pointer_constraints_v1_interface.name) == 0) {
        window->pointer_constraints = wl_registry_bind(registry, name, &zwp_pointer_constraints_v1_interface, 1);
    } else if (strcmp(interface, zwp_relative_pointer_manager_v1_interface.name) == 0) {
        window->relative_pointer_manager = wl_registry_bind(registry, name, &zwp_relative_pointer_manager_v1_interface, 1);
    }
}
static const struct wl_registry_listener registry_listener = { .global = registry_handle_global };

// --- Window Listeners ---
static void toplevel_handle_configure(void* data, struct xdg_toplevel* t, int32_t w, int32_t h, struct wl_array* s) {
    /* (void) cast safely suppresses unused parameter warnings for mandatory signatures */
    (void)t;
    (void)s;
    sbgl_Window* win = (sbgl_Window*)data;
    if (w > 0 && h > 0 && (w != win->width || h != win->height)) {
        win->width = w;
        win->height = h;
        win->resized = true;
    }
}
static void toplevel_handle_close(void* data, struct xdg_toplevel* t) {
    /* (void) cast safely suppresses unused parameter warnings for mandatory signatures */
    (void)t;
    ((sbgl_Window*)data)->shouldClose = true;
}
static const struct xdg_toplevel_listener toplevel_listener = { .configure = toplevel_handle_configure, .close = toplevel_handle_close };

static void xdg_surface_handle_configure(void* data, struct xdg_surface* surf, uint32_t serial) {
    /* (void) cast safely suppresses unused parameter warnings for mandatory signatures */
    (void)data;
    xdg_surface_ack_configure(surf, serial);
}
static const struct xdg_surface_listener xdg_surface_listener = { .configure = xdg_surface_handle_configure };

// --- Pointer Constraints ---
static void locked_pointer_locked(void* data, struct zwp_locked_pointer_v1* locked_pointer) { (void)data; (void)locked_pointer; }
static void locked_pointer_unlocked(void* data, struct zwp_locked_pointer_v1* locked_pointer) { (void)data; (void)locked_pointer; }
static const struct zwp_locked_pointer_v1_listener locked_pointer_listener = { .locked = locked_pointer_locked, .unlocked = locked_pointer_unlocked };

// --- HAL ---
sbgl_Window* sbgl_os_CreateWindow(struct SblArena* arena, sbgl_InputState* input, int width, int height, const char* title) {
    sbgl_Window* window = SBL_ARENA_PUSH_STRUCT_ZERO(arena, sbgl_Window);
    if (!window) return NULL;
    window->width = width; window->height = height;
    window->shouldClose = false;
    window->resized = false;
    window->focused = false;
    window->cursor_visible = true;
    window->input = input;

    window->display = wl_display_connect(NULL);
    if (!window->display) return NULL;

    struct wl_registry* reg = wl_display_get_registry(window->display);
    wl_registry_add_listener(reg, &registry_listener, window);
    wl_display_roundtrip(window->display);

    window->surface = wl_compositor_create_surface(window->compositor);
    window->xdg_surface = xdg_wm_base_get_xdg_surface(window->wm_base, window->surface);
    xdg_surface_add_listener(window->xdg_surface, &xdg_surface_listener, window);
    window->xdg_toplevel = xdg_surface_get_toplevel(window->xdg_surface);
    xdg_toplevel_add_listener(window->xdg_toplevel, &toplevel_listener, window);
    xdg_toplevel_set_title(window->xdg_toplevel, title);

    if (window->seat) {
        struct wl_keyboard* k = wl_seat_get_keyboard(window->seat);
        if (k) wl_keyboard_add_listener(k, &keyboard_listener, window);
        struct wl_pointer* p = wl_seat_get_pointer(window->seat);
        if (p) wl_pointer_add_listener(p, &pointer_listener, window);
    }

    wl_surface_commit(window->surface);
    wl_display_roundtrip(window->display);
    return window;
}

void sbgl_os_PollEvents(sbgl_Window* window) {
    if (!window) return;
    
    // Clear deltas if relative pointer is active to allow accumulation in listeners
    if (window->relative_pointer && window->input) {
        window->input->mouseDeltaX = 0;
        window->input->mouseDeltaY = 0;
    }

    while (wl_display_prepare_read(window->display) != 0) wl_display_dispatch_pending(window->display);
    wl_display_flush(window->display);
    wl_display_read_events(window->display);
    wl_display_dispatch_pending(window->display);
    linux_internal_update_input_states(window);
}

bool sbgl_os_WindowShouldClose(sbgl_Window* window) { return window->shouldClose; }
void sbgl_os_GetWindowSize(sbgl_Window* window, int* w, int* h) { *w = window->width; *h = window->height; }

bool sbgl_os_WasWindowResized(sbgl_Window* window) {
    bool r = window->resized;
    window->resized = false;
    return r;
}

bool sbgl_os_IsWindowFocused(sbgl_Window* window) { return window->focused; }

void sbgl_os_SetCursorVisible(sbgl_Window* window, bool visible) {
    if (!window) return;
    window->cursor_visible = visible;
    
    if (!visible && window->seat) {
        struct wl_pointer* pointer = wl_seat_get_pointer(window->seat);
        if (pointer) {
            // Setting the cursor surface to NULL hides the cursor for this surface.
            wl_pointer_set_cursor(pointer, window->pointer_serial, NULL, 0, 0);
        }
    }
}

extern const struct zwp_relative_pointer_v1_listener relative_pointer_listener;

void sbgl_os_SetCursorLocked(sbgl_Window* window, bool locked) {
    if (!window || !window->seat || !window->pointer_constraints) return;
    struct wl_pointer* pointer = wl_seat_get_pointer(window->seat);
    if (!pointer) return;

    if (locked && !window->locked_pointer) {
        window->locked_pointer = zwp_pointer_constraints_v1_lock_pointer(
            window->pointer_constraints, window->surface, pointer, NULL,
            ZWP_POINTER_CONSTRAINTS_V1_LIFETIME_PERSISTENT);
        zwp_locked_pointer_v1_add_listener(window->locked_pointer, &locked_pointer_listener, window);

        if (window->relative_pointer_manager) {
            window->relative_pointer = zwp_relative_pointer_manager_v1_get_relative_pointer(
                window->relative_pointer_manager, pointer);
            zwp_relative_pointer_v1_add_listener(window->relative_pointer, &relative_pointer_listener, window);
        }
    } else if (!locked && window->locked_pointer) {
        zwp_locked_pointer_v1_destroy(window->locked_pointer);
        window->locked_pointer = NULL;
        if (window->relative_pointer) {
            zwp_relative_pointer_v1_destroy(window->relative_pointer);
            window->relative_pointer = NULL;
        }
    }
}

uint64_t sbgl_os_GetPerfCount(void) { struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec; }
uint64_t sbgl_os_GetPerfFreq(void) { return 1000000000ULL; }
void* sbgl_os_GetNativeWindowHandle(sbgl_Window* window) { return (void*)window->surface; }
void* sbgl_os_GetNativeDisplayHandle(sbgl_Window* window) { return (void*)window->display; }
void sbgl_os_DestroyWindow(sbgl_Window* window) { 
    if (!window) return;
    if (window->relative_pointer) zwp_relative_pointer_v1_destroy(window->relative_pointer);
    if (window->locked_pointer) zwp_locked_pointer_v1_destroy(window->locked_pointer);
    if (window->relative_pointer_manager) zwp_relative_pointer_manager_v1_destroy(window->relative_pointer_manager);
    if (window->pointer_constraints) zwp_pointer_constraints_v1_destroy(window->pointer_constraints);
    xdg_toplevel_destroy(window->xdg_toplevel); 
    xdg_surface_destroy(window->xdg_surface); 
    wl_surface_destroy(window->surface); 
    wl_display_disconnect(window->display);
}
