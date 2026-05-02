#include "linux_internal.h"
#include <string.h>

// Forward declaration of the window struct to access input
struct sbgl_Window {
    void* surface;
    void* xdg_surface;
    void* xdg_toplevel;
    sbgl_InputState* input;
};

static SBGL_Scancode wayland_key_to_scancode(uint32_t key) {
    switch (key) {
        case 1:   return SBGL_SCANCODE_ESCAPE;
        case 28:  return SBGL_SCANCODE_RETURN;
        case 57:  return SBGL_SCANCODE_SPACE;
        case 15:  return SBGL_SCANCODE_TAB;
        case 14:  return SBGL_SCANCODE_BACKSPACE;
        case 103: return SBGL_SCANCODE_UP;
        case 108: return SBGL_SCANCODE_DOWN;
        case 105: return SBGL_SCANCODE_LEFT;
        case 106: return SBGL_SCANCODE_RIGHT;
        case 42:  return SBGL_SCANCODE_LSHIFT;
        case 29:  return SBGL_SCANCODE_LCTRL;
        case 56:  return SBGL_SCANCODE_LALT;
        case 30: return SBGL_SCANCODE_A; case 48: return SBGL_SCANCODE_B; case 46: return SBGL_SCANCODE_C;
        case 32: return SBGL_SCANCODE_D; case 18: return SBGL_SCANCODE_E; case 33: return SBGL_SCANCODE_F;
        case 34: return SBGL_SCANCODE_G; case 35: return SBGL_SCANCODE_H; case 23: return SBGL_SCANCODE_I;
        case 36: return SBGL_SCANCODE_J; case 37: return SBGL_SCANCODE_K; case 38: return SBGL_SCANCODE_L;
        case 50: return SBGL_SCANCODE_M; case 49: return SBGL_SCANCODE_N; case 24: return SBGL_SCANCODE_O;
        case 25: return SBGL_SCANCODE_P; case 16: return SBGL_SCANCODE_Q; case 19: return SBGL_SCANCODE_R;
        case 31: return SBGL_SCANCODE_S; case 20: return SBGL_SCANCODE_T; case 22: return SBGL_SCANCODE_U;
        case 47: return SBGL_SCANCODE_V; case 17: return SBGL_SCANCODE_W; case 45: return SBGL_SCANCODE_X;
        case 21: return SBGL_SCANCODE_Y; case 44: return SBGL_SCANCODE_Z;
        default: return SBGL_SCANCODE_UNKNOWN;
    }
}

// --- Pointer Listeners ---
static void pointer_enter(void* data, struct wl_pointer* p, uint32_t s, struct wl_surface* surf, wl_fixed_t sx, wl_fixed_t sy) {
    (void)p; (void)s; (void)surf;
    sbgl_InputState* input = ((struct sbgl_Window*)data)->input;
    input->mouseX = wl_fixed_to_int(sx);
    input->mouseY = wl_fixed_to_int(sy);
}
static void pointer_leave(void* data, struct wl_pointer* p, uint32_t s, struct wl_surface* surf) { (void)data; (void)p; (void)s; (void)surf; }
static void pointer_motion(void* data, struct wl_pointer* p, uint32_t t, wl_fixed_t sx, wl_fixed_t sy) {
    (void)p; (void)t;
    sbgl_InputState* input = ((struct sbgl_Window*)data)->input;
    input->mouseX = wl_fixed_to_int(sx);
    input->mouseY = wl_fixed_to_int(sy);
}
static void pointer_button(void* data, struct wl_pointer* p, uint32_t s, uint32_t t, uint32_t button, uint32_t state) {
    (void)p; (void)s; (void)t;
    sbgl_InputState* input = ((struct sbgl_Window*)data)->input;
    int btn = -1;
    if (button == 0x110) btn = SBGL_MOUSE_BUTTON_LEFT;
    else if (button == 0x111) btn = SBGL_MOUSE_BUTTON_RIGHT;
    else if (button == 0x112) btn = SBGL_MOUSE_BUTTON_MIDDLE;
    if (btn != -1 && btn < SBGL_MAX_MOUSE_BUTTONS) {
        input->mouseDown[btn] = (state == WL_POINTER_BUTTON_STATE_PRESSED);
    }
}
static void pointer_axis(void* data, struct wl_pointer* p, uint32_t t, uint32_t axis, wl_fixed_t value) { (void)data; (void)p; (void)t; (void)axis; (void)value; }
static void pointer_frame(void* data, struct wl_pointer* p) { (void)data; (void)p; }
static void pointer_axis_source(void* data, struct wl_pointer* p, uint32_t src) { (void)data; (void)p; (void)src; }
static void pointer_axis_stop(void* data, struct wl_pointer* p, uint32_t t, uint32_t axis) { (void)data; (void)p; (void)t; (void)axis; }
static void pointer_axis_discrete(void* data, struct wl_pointer* p, uint32_t axis, int32_t discrete) { (void)data; (void)p; (void)axis; (void)discrete; }

const struct wl_pointer_listener pointer_listener = {
    .enter = pointer_enter, .leave = pointer_leave, .motion = pointer_motion, .button = pointer_button,
    .axis = pointer_axis, .frame = pointer_frame, .axis_source = pointer_axis_source,
    .axis_stop = pointer_axis_stop, .axis_discrete = pointer_axis_discrete
};

// --- Keyboard Listeners ---
static void keyboard_keymap(void* data, struct wl_keyboard* k, uint32_t format, int32_t fd, uint32_t size) { (void)data; (void)k; (void)format; (void)fd; (void)size; }
static void keyboard_enter(void* data, struct wl_keyboard* k, uint32_t s, struct wl_surface* surf, struct wl_array* keys) { (void)data; (void)k; (void)s; (void)surf; (void)keys; }
static void keyboard_leave(void* data, struct wl_keyboard* k, uint32_t s, struct wl_surface* surf) { (void)data; (void)k; (void)s; (void)surf; }
static void keyboard_key(void* data, struct wl_keyboard* k, uint32_t s, uint32_t t, uint32_t key, uint32_t state) {
    (void)k; (void)s; (void)t;
    sbgl_InputState* input = ((struct sbgl_Window*)data)->input;
    SBGL_Scancode code = wayland_key_to_scancode(key);
    if (code < SBGL_MAX_KEYS) {
        bool down = (state == WL_KEYBOARD_KEY_STATE_PRESSED);
        if (down && !input->keysDown[code]) {
            input->keysPressed[code] = true;
        }
        input->keysDown[code] = down;
    }
}
static void keyboard_modifiers(void* data, struct wl_keyboard* k, uint32_t s, uint32_t depressed, uint32_t latched, uint32_t locked, uint32_t group) { (void)data; (void)k; (void)s; (void)depressed; (void)latched; (void)locked; (void)group; }
static void keyboard_repeat_info(void* data, struct wl_keyboard* k, int32_t rate, int32_t delay) { (void)data; (void)k; (void)rate; (void)delay; }

const struct wl_keyboard_listener keyboard_listener = {
    .keymap = keyboard_keymap, .enter = keyboard_enter, .leave = keyboard_leave,
    .key = keyboard_key, .modifiers = keyboard_modifiers, .repeat_info = keyboard_repeat_info
};

void linux_init_input(struct wl_registry* registry, uint32_t name, uint32_t version) {
    g_seat = wl_registry_bind(registry, name, &wl_seat_interface, version);
}

// --- HAL Implementation ---
void linux_internal_update_input_states(sbgl_InputState* input) {
    if (!input) return;
    input->mouseDeltaX = input->mouseX - input->_internalMouseX;
    input->mouseDeltaY = input->mouseY - input->_internalMouseY;
    input->_internalMouseX = input->mouseX;
    input->_internalMouseY = input->mouseY;
}
