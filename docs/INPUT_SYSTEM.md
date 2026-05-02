# SBgl Input System Architecture

The input system in SBgl is designed for high-performance, real-time access with zero runtime function overhead. It utilizes a "key map" pattern tied directly to the engine context.

---

## Contextual Encapsulation
Unlike traditional engines that use global state or OS-specific polling, SBgl stores all physical input state within the `sbgl_Context`. 

```c
typedef struct sbgl_Context {
    void*           inner;
    sbgl_Result     result;
    sbgl_InputState input; // <--- Physical state lives here
} sbgl_Context;
```

This design ensures that:
- Input state is thread-safe relative to the context.
- The state is always available without making expensive system calls.
- Multiple windows can have independent input states.

## The Key Map Pattern
The `sbgl_InputState` structure contains fixed-size boolean arrays for every possible physical key and mouse button.

- **`keysDown`**: A bit-array or boolean array representing the current physical state of keys.
- **`keysPressed`**: A specialized array that is set to `true` only on the frame a key was first pressed.
- **`mouseDown`**: Current state of the mouse buttons.

When the OS sends a message (e.g., `WM_KEYDOWN` on Windows or `wl_keyboard.key` on Wayland), the **Platform Layer** translates the native scancode into an SBgl scancode and updates the corresponding index in these arrays.

## Input HAL Implementation
The Platform Layer (Wayland, X11, Win32) is responsible for filling the key map during the `sbgl_os_PollEvents` phase.

### Mouse Deltas
Mouse movement is tracked using absolute coordinates (`mouseX`, `mouseY`) and relative deltas. Deltas are calculated during the event poll by comparing the current position with the position from the previous frame stored in internal tracking variables within the `sbgl_InputState` (`_internalMouseX`, `_internalMouseY`). This ensures deltas are calculated accurately even when frames are skipped or delayed.

### Frame Lifecycle
Pressed states (`keysPressed`) are automatically reset at the end of every frame in `sbgl_EndDrawing`. This ensures that a "pressed" event is only visible to the user for exactly one frame.

## User Usage
Users can access input in two ways:

### High-Level API (Helper Functions)
```c
if (sbgl_IsKeyDown(ctx, SBGL_KEY_SPACE)) {
    // Jump logic
}
```
These functions perform safety and bounds checks before returning the state.

### Direct Access (Zero Overhead)
For maximum performance, the user can read the arrays directly:
```c
if (ctx->input.keysDown[SBGL_KEY_W]) {
    // Move forward
}
```

---

## Technical Summary
| Feature | Implementation | Benefit |
| :--- | :--- | :--- |
| **State Storage** | Embedded in `sbgl_Context` | Thread-safety and cache locality. |
| **Polling** | Event-driven (Async) | Zero OS overhead during frame logic. |
| **Deltas** | Frame-to-frame comparison | Perfect for camera controls. |
| **Memory** | Fixed arrays | No dynamic allocation during input events. |
