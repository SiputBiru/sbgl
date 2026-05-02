#ifndef SBGL_H
#define SBGL_H

#include <stdbool.h>
#include <stdint.h>

// --- Scancodes (Subset of physical keys) ---
#define SBGL_KEY_UNKNOWN 0
#define SBGL_KEY_A 4
#define SBGL_KEY_B 5
#define SBGL_KEY_C 6
#define SBGL_KEY_D 7
#define SBGL_KEY_E 8
#define SBGL_KEY_F 9
#define SBGL_KEY_G 10
#define SBGL_KEY_H 11
#define SBGL_KEY_I 12
#define SBGL_KEY_J 13
#define SBGL_KEY_K 14
#define SBGL_KEY_L 15
#define SBGL_KEY_M 16
#define SBGL_KEY_N 17
#define SBGL_KEY_O 18
#define SBGL_KEY_P 19
#define SBGL_KEY_Q 20
#define SBGL_KEY_R 21
#define SBGL_KEY_S 22
#define SBGL_KEY_T 23
#define SBGL_KEY_U 24
#define SBGL_KEY_V 25
#define SBGL_KEY_W 26
#define SBGL_KEY_X 27
#define SBGL_KEY_Y 28
#define SBGL_KEY_Z 29

#define SBGL_KEY_1 30
#define SBGL_KEY_2 31
#define SBGL_KEY_3 32
#define SBGL_KEY_4 33
#define SBGL_KEY_5 34
#define SBGL_KEY_6 35
#define SBGL_KEY_7 36
#define SBGL_KEY_8 37
#define SBGL_KEY_9 38
#define SBGL_KEY_0 39

#define SBGL_KEY_RETURN 40
#define SBGL_KEY_ESCAPE 41
#define SBGL_KEY_BACKSPACE 42
#define SBGL_KEY_TAB 43
#define SBGL_KEY_SPACE 44
#define SBGL_KEY_RIGHT 79
#define SBGL_KEY_LEFT 80
#define SBGL_KEY_DOWN 81
#define SBGL_KEY_UP 82
#define SBGL_KEY_LSHIFT 225
#define SBGL_KEY_LCTRL 224
#define SBGL_KEY_LALT 226

#define SBGL_MOUSE_LEFT 0
#define SBGL_MOUSE_RIGHT 1
#define SBGL_MOUSE_MIDDLE 2

// --- Types ---

typedef enum {
    SBGL_SUCCESS = 0,
    SBGL_ERROR_INITIALIZATION_FAILED = 1,
    SBGL_ERROR_WINDOW_CREATION_FAILED = 2,
    SBGL_ERROR_GRAPHICS_INITIALIZATION_FAILED = 3,
    SBGL_ERROR_OUT_OF_MEMORY = 4,
} sbgl_Result;

typedef struct sbgl_Context {
    void*       inner;
    sbgl_Result result;
} sbgl_Context;

// --- API ---

// Lifecycle
sbgl_Context* sbgl_Init(int w, int h, const char* title);
void          sbgl_Shutdown(sbgl_Context* ctx);
bool          sbgl_WindowShouldClose(sbgl_Context* ctx);

// Graphics Lifecycle
void sbgl_BeginDrawing(sbgl_Context* ctx);
void sbgl_EndDrawing(sbgl_Context* ctx);
void sbgl_Clear(sbgl_Context* ctx, float r, float g, float b, float a);

// Input - Keyboard
bool sbgl_IsKeyDown(sbgl_Context* ctx, int scancode);
bool sbgl_IsKeyPressed(sbgl_Context* ctx, int scancode);

// Input - Mouse
bool sbgl_IsMouseButtonDown(sbgl_Context* ctx, int button);
void sbgl_GetMousePos(sbgl_Context* ctx, int* x, int* y);
void sbgl_GetMouseDelta(sbgl_Context* ctx, int* dx, int* dy);

#endif // SBGL_H
