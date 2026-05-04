/**
 * @file sbgl_input.h
 * @brief Public input types and scancodes for SBgl.
 *
 * Provides a platform-independent set of physical scancodes and mouse button
 * definitions, along with the complete input state structure.
 */

#ifndef SBGL_PUBLIC_INPUT_H
#define SBGL_PUBLIC_INPUT_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief OS-independent physical scancodes.
 *
 * These match the USB HID usage table and are used for frame-accurate
 * input detection.
 */
typedef enum {
    SBGL_SCANCODE_UNKNOWN = 0,
    SBGL_SCANCODE_A = 4,
    SBGL_SCANCODE_B = 5,
    SBGL_SCANCODE_C = 6,
    SBGL_SCANCODE_D = 7,
    SBGL_SCANCODE_E = 8,
    SBGL_SCANCODE_F = 9,
    SBGL_SCANCODE_G = 10,
    SBGL_SCANCODE_H = 11,
    SBGL_SCANCODE_I = 12,
    SBGL_SCANCODE_J = 13,
    SBGL_SCANCODE_K = 14,
    SBGL_SCANCODE_L = 15,
    SBGL_SCANCODE_M = 16,
    SBGL_SCANCODE_N = 17,
    SBGL_SCANCODE_O = 18,
    SBGL_SCANCODE_P = 19,
    SBGL_SCANCODE_Q = 20,
    SBGL_SCANCODE_R = 21,
    SBGL_SCANCODE_S = 22,
    SBGL_SCANCODE_T = 23,
    SBGL_SCANCODE_U = 24,
    SBGL_SCANCODE_V = 25,
    SBGL_SCANCODE_W = 26,
    SBGL_SCANCODE_X = 27,
    SBGL_SCANCODE_Y = 28,
    SBGL_SCANCODE_Z = 29,
    
    SBGL_SCANCODE_1 = 30,
    SBGL_SCANCODE_2 = 31,
    SBGL_SCANCODE_3 = 32,
    SBGL_SCANCODE_4 = 33,
    SBGL_SCANCODE_5 = 34,
    SBGL_SCANCODE_6 = 35,
    SBGL_SCANCODE_7 = 36,
    SBGL_SCANCODE_8 = 37,
    SBGL_SCANCODE_9 = 38,
    SBGL_SCANCODE_0 = 39,

    SBGL_SCANCODE_RETURN = 40,
    SBGL_SCANCODE_ESCAPE = 41,
    SBGL_SCANCODE_BACKSPACE = 42,
    SBGL_SCANCODE_TAB = 43,
    SBGL_SCANCODE_SPACE = 44,

    SBGL_SCANCODE_RIGHT = 79,
    SBGL_SCANCODE_LEFT = 80,
    SBGL_SCANCODE_DOWN = 81,
    SBGL_SCANCODE_UP = 82,

    SBGL_SCANCODE_LSHIFT = 225,
    SBGL_SCANCODE_LCTRL = 224,
    SBGL_SCANCODE_LALT = 226,
    
    SBGL_SCANCODE_MAX = 512
} SBGL_Scancode;

/**
 * @brief Standard mouse buttons.
 */
typedef enum {
    SBGL_MOUSE_BUTTON_LEFT = 0,
    SBGL_MOUSE_BUTTON_RIGHT = 1,
    SBGL_MOUSE_BUTTON_MIDDLE = 2,
    SBGL_MOUSE_BUTTON_MAX = 8
} SBGL_MouseButton;

/**
 * @brief Represents the real-time state of physical inputs.
 *
 * Adheres to Data-Oriented Design by storing flat arrays of booleans for 
 * instant lookup and cache-friendly batch processing.
 */
typedef struct sbgl_InputState {
    bool keysDown[SBGL_SCANCODE_MAX];      /**< Current physical state of keys. */
    bool keysPressed[SBGL_SCANCODE_MAX];   /**< Set once when key is pressed, reset every frame. */
    bool mouseDown[SBGL_MOUSE_BUTTON_MAX]; /**< Current physical state of mouse buttons. */
    int mouseX, mouseY;                    /**< Absolute window coordinates. */
    int mouseDeltaX, mouseDeltaY;          /**< Relative motion since last frame. */
    int _internalMouseX, _internalMouseY;  /**< Private tracking for delta calculation. */
} sbgl_InputState;

#endif // SBGL_PUBLIC_INPUT_H
