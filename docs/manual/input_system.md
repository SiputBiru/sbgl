# SBgl Input System Architecture

The input system provides access to physical device state through the engine context. It utilizes an internal "key map" pattern tied to the engine context.

---

## Contextual Encapsulation
Unlike systems that use global state or OS-specific polling, SBgl stores all physical input state within the opaque `sbgl_InternalContext`. 

```c
typedef struct sbgl_Context {
    void*           inner; // <--- Physical state lives inside here
    sbgl_Result     result;
} sbgl_Context;
```

This design ensures that:
- Input state is thread-safe relative to the context.
- The state is always available internally without making system calls.
- Multiple windows can have independent input states.
- The public API remains strictly controlled.

## The Key Map Pattern
Internally, the `sbgl_InputState` structure contains fixed-size boolean arrays for every possible physical key and mouse button.

- **`keysDown`**: A bit-array or boolean array representing the current physical state of keys.
- **`keysPressed`**: A specialized array that is set to `true` only on the frame a key was first pressed.
- **`mouseDown`**: Current state of the mouse buttons.

When the OS sends a message (e.g., `WM_KEYDOWN` on Windows or `wl_keyboard.key` on Wayland), the **Platform Layer** translates the native scancode into an SBgl scancode (defined in `sbgl.h`) and updates the corresponding index in these internal arrays.

## Input HAL Implementation
The Platform Layer (Wayland, X11, Win32) is responsible for filling the key map during the `sbgl_os_PollEvents` phase.

### Mouse Deltas
Mouse movement is tracked using absolute coordinates (`mouseX`, `mouseY`) and relative deltas. Deltas are calculated during the event poll by comparing the current position with the position from the previous frame stored in internal tracking variables within the `sbgl_InputState` (`_internalMouseX`, `_internalMouseY`). This ensures deltas are calculated accurately even when frames are skipped or delayed.

### Frame Lifecycle
Pressed states (`keysPressed`) are automatically reset at the end of every frame in `sbgl_EndDrawing`. This ensures that a "pressed" event is only visible to the application for exactly one frame.

## Application Integration
The system provides a Data-Oriented API for batch processing.

### Data-Oriented API (Batch Access)
The `sbgl_GetInputState` function provides a read-only pointer to the contiguous `sbgl_InputState` structure. Fetching the pointer once per frame enables batch processing.

#### Keyboard Example
The `keysDown` (held) or `keysPressed` (one-shot) boolean arrays are queried directly.
```c
const sbgl_InputState* input = sbgl_GetInputState(ctx);

// Continuous movement check
if (input->keysDown[SBGL_KEY_W]) pos_y -= speed;

// One-shot toggle check
if (input->keysPressed[SBGL_KEY_SPACE]) is_jumping = true;
```

#### Mouse Example
Access real-time coordinates, deltas, and button states.
```c
const sbgl_InputState* input = sbgl_GetInputState(ctx);

// Direct access to coordinates and movement deltas
int x = input->mouseX;
int y = input->mouseY;
int dx = input->mouseDeltaX;

// Mouse button checks
if (input->mouseDown[SBGL_MOUSE_LEFT]) {
    spawn_particle_at(x, y);
}
```

The design adheres to Data-Oriented Design principles by:
- **Maximizing Cache Efficiency**: Providing direct access to the underlying arrays (`keysDown`, `keysPressed`, `mouseDown`) allows the CPU to prefetch data effectively during batch processing loops.
- **Eliminating Call Overhead**: Fetching the entire state once per frame avoids the overhead of multiple function calls for individual key checks.
- **Ensuring Stability**: The API returns a pointer to a static, zero-initialized dummy state if the context is invalid, preventing null-pointer dereferences.

By exposing the state as a contiguous block of data, the engine enables iteration and transformation of input data without the overhead of object-oriented getters.

---

## Technical Summary
| Feature | Implementation | Benefit |
| :--- | :--- | :--- |
| **State Storage** | Embedded in `sbgl_InternalContext` | Strict encapsulation and thread-safety. |
| **Polling** | Event-driven (Async) | Zero OS overhead during frame logic. |
| **Data Layout** | Tightly packed fixed arrays | Cache efficiency and batch-processing support. |
| **Memory** | O(1) Allocation (Arena) | No dynamic allocation during input events. |
