#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "../include/sbgl_input.h"

static void test_input_state_layout(void) {
    sbgl_InputState state;
    memset(&state, 0, sizeof(state));

    printf("InputState Size: %zu bytes\n", sizeof(sbgl_InputState));
    
    // Check scancode bounds
    assert(SBGL_SCANCODE_MAX == 512);
    assert(sizeof(state.keysDown) == 512 * sizeof(bool));
    assert(sizeof(state.keysPressed) == 512 * sizeof(bool));
    
    // Check mouse button bounds
    assert(SBGL_MOUSE_BUTTON_MAX == 8);
    assert(sizeof(state.mouseDown) == 8 * sizeof(bool));
    
    // Verify that the internal mouse tracking is present
    state._internalMouseX = 10;
    state._internalMouseY = 20;
    assert(state._internalMouseX == 10);
    assert(state._internalMouseY == 20);
}

static void test_scancodes(void) {
    // Verify some key scancodes against USB HID expectations.
    assert(SBGL_SCANCODE_UNKNOWN == 0);
    assert(SBGL_SCANCODE_A == 4);
    assert(SBGL_SCANCODE_B == 5);
    assert(SBGL_SCANCODE_C == 6);
    assert(SBGL_SCANCODE_D == 7);
    assert(SBGL_SCANCODE_E == 8);
    assert(SBGL_SCANCODE_F == 9);
    assert(SBGL_SCANCODE_G == 10);
    assert(SBGL_SCANCODE_H == 11);
    assert(SBGL_SCANCODE_I == 12);
    assert(SBGL_SCANCODE_J == 13);
    assert(SBGL_SCANCODE_K == 14);
    assert(SBGL_SCANCODE_L == 15);
    assert(SBGL_SCANCODE_M == 16);
    assert(SBGL_SCANCODE_N == 17);
    assert(SBGL_SCANCODE_O == 18);
    assert(SBGL_SCANCODE_P == 19);
    assert(SBGL_SCANCODE_Q == 20);
    assert(SBGL_SCANCODE_R == 21);
    assert(SBGL_SCANCODE_S == 22);
    assert(SBGL_SCANCODE_T == 23);
    assert(SBGL_SCANCODE_U == 24);
    assert(SBGL_SCANCODE_V == 25);
    assert(SBGL_SCANCODE_W == 26);
    assert(SBGL_SCANCODE_X == 27);
    assert(SBGL_SCANCODE_Y == 28);
    assert(SBGL_SCANCODE_Z == 29);
    
    assert(SBGL_SCANCODE_1 == 30);
    assert(SBGL_SCANCODE_0 == 39);

    assert(SBGL_SCANCODE_RETURN == 40);
    assert(SBGL_SCANCODE_ESCAPE == 41);
    assert(SBGL_SCANCODE_BACKSPACE == 42);
    assert(SBGL_SCANCODE_TAB == 43);
    assert(SBGL_SCANCODE_SPACE == 44);

    assert(SBGL_SCANCODE_RIGHT == 79);
    assert(SBGL_SCANCODE_LEFT == 80);
    assert(SBGL_SCANCODE_DOWN == 81);
    assert(SBGL_SCANCODE_UP == 82);

    assert(SBGL_SCANCODE_LSHIFT == 225);
    assert(SBGL_SCANCODE_LCTRL == 224);
    assert(SBGL_SCANCODE_LALT == 226);
}

static void test_mouse_buttons(void) {
    assert(SBGL_MOUSE_BUTTON_LEFT == 0);
    assert(SBGL_MOUSE_BUTTON_RIGHT == 1);
    assert(SBGL_MOUSE_BUTTON_MIDDLE == 2);
}

static void test_mouse_modes(void) {
    assert(SBGL_MOUSE_MODE_NORMAL == 0);
    assert(SBGL_MOUSE_MODE_HIDDEN == 1);
    assert(SBGL_MOUSE_MODE_CAPTURED == 2);
}

int main(void) {
    printf("--- SBgl Input Test ---\n");
    test_input_state_layout();
    test_scancodes();
    test_mouse_buttons();
    test_mouse_modes();
    printf("All input tests passed!\n");
    return 0;
}
