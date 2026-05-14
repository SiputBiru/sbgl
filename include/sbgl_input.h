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

    SBGL_SCANCODE_MINUS = 45,
    SBGL_SCANCODE_EQUAL = 46,

    SBGL_SCANCODE_RIGHT = 79,
    SBGL_SCANCODE_LEFT = 80,
    SBGL_SCANCODE_DOWN = 81,
    SBGL_SCANCODE_UP = 82,

    SBGL_SCANCODE_LSHIFT = 225,
    SBGL_SCANCODE_LCTRL = 224,
    SBGL_SCANCODE_LALT = 226,
    
    /* Function keys */
    SBGL_SCANCODE_F1 = 58,
    SBGL_SCANCODE_F2 = 59,
    SBGL_SCANCODE_F3 = 60,
    SBGL_SCANCODE_F4 = 61,
    SBGL_SCANCODE_F5 = 62,
    SBGL_SCANCODE_F6 = 63,
    SBGL_SCANCODE_F7 = 64,
    SBGL_SCANCODE_F8 = 65,
    SBGL_SCANCODE_F9 = 66,
    SBGL_SCANCODE_F10 = 67,
    SBGL_SCANCODE_F11 = 68,
    SBGL_SCANCODE_F12 = 69,
    
    /* Navigation keys */
    SBGL_SCANCODE_INSERT = 73,
    SBGL_SCANCODE_HOME = 74,
    SBGL_SCANCODE_PAGEUP = 75,
    SBGL_SCANCODE_DELETE = 76,
    SBGL_SCANCODE_END = 77,
    SBGL_SCANCODE_PAGEDOWN = 78,
    
    /* Numpad */
    SBGL_SCANCODE_NUMLOCKCLEAR = 83,
    SBGL_SCANCODE_KP_DIVIDE = 84,
    SBGL_SCANCODE_KP_MULTIPLY = 85,
    SBGL_SCANCODE_KP_MINUS = 86,
    SBGL_SCANCODE_KP_PLUS = 87,
    SBGL_SCANCODE_KP_ENTER = 88,
    SBGL_SCANCODE_KP_1 = 89,
    SBGL_SCANCODE_KP_2 = 90,
    SBGL_SCANCODE_KP_3 = 91,
    SBGL_SCANCODE_KP_4 = 92,
    SBGL_SCANCODE_KP_5 = 93,
    SBGL_SCANCODE_KP_6 = 94,
    SBGL_SCANCODE_KP_7 = 95,
    SBGL_SCANCODE_KP_8 = 96,
    SBGL_SCANCODE_KP_9 = 97,
    SBGL_SCANCODE_KP_0 = 98,
    SBGL_SCANCODE_KP_PERIOD = 99,
    
    /* Symbol keys */
    SBGL_SCANCODE_SEMICOLON = 51,
    SBGL_SCANCODE_APOSTROPHE = 52,
    SBGL_SCANCODE_GRAVE = 53,
    SBGL_SCANCODE_COMMA = 54,
    SBGL_SCANCODE_PERIOD = 55,
    SBGL_SCANCODE_SLASH = 56,
    SBGL_SCANCODE_CAPSLOCK = 57,
    SBGL_SCANCODE_LEFTBRACKET = 47,
    SBGL_SCANCODE_BACKSLASH = 49,
    SBGL_SCANCODE_RIGHTBRACKET = 48,
    
    /* Right modifiers */
    SBGL_SCANCODE_RSHIFT = 229,
    SBGL_SCANCODE_RCTRL = 228,
    SBGL_SCANCODE_RALT = 230,
    
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
 * @brief Mouse behavior modes.
 */
typedef enum {
    SBGL_MOUSE_MODE_NORMAL,   /**< Standard OS cursor, visible and free. */
    SBGL_MOUSE_MODE_HIDDEN,   /**< Cursor is invisible but moves freely. */
    SBGL_MOUSE_MODE_CAPTURED  /**< Cursor is invisible and locked to window center. */
} sbgl_MouseMode;

/**
 * @brief Represents the real-time state of physical inputs.
 *
 * Adheres to Data-Oriented Design by storing flat arrays of booleans for 
 * O(1) lookup and contiguous memory access during batch processing.
 */
typedef struct sbgl_InputState {
    bool keysDown[SBGL_SCANCODE_MAX];      /**< Physical state of keys. */
    bool keysPressed[SBGL_SCANCODE_MAX];   /**< Set once when key is pressed, reset every frame. */
    bool mouseDown[SBGL_MOUSE_BUTTON_MAX]; /**< Physical state of mouse buttons. */
    int mouseX, mouseY;                    /**< Absolute window coordinates. */
    int mouseDeltaX, mouseDeltaY;          /**< Relative motion since last frame. */
    int _internalMouseX, _internalMouseY;  /**< Private tracking for delta calculation. */
} sbgl_InputState;

#endif // SBGL_PUBLIC_INPUT_H
