# Windows-Specific Features and Behaviors

This document describes Windows-specific implementations, behaviors, and considerations when using SBgl on Win32 platforms. Understanding these details helps ensure optimal performance and correct behavior on Windows systems.

---

## Table of Contents

1. [Window Class Registration](#window-class-registration)
2. [Cursor Locking and Raw Input](#cursor-locking-and-raw-input)
3. [DPI Awareness](#dpi-awareness)
4. [Console vs Window Subsystem](#console-vs-window-subsystem)
5. [Error Reporting](#error-reporting)
6. [Key Repeat Behavior](#key-repeat-behavior)
7. [Scancode Mapping](#scancode-mapping)
8. [Client Area Size](#client-area-size)

---

## Window Class Registration

### Overview

On Windows, SBgl registers a window class named `"SBglWindowClass"` during `sbgl_os_CreateWindow()`. This class defines the window's behavior, including:

* Window procedure (event handling)
* Cursor type (arrow by default)
* Background brush
* Icon and menu (none by default)

### Lifecycle

```
sbgl_os_CreateWindow()
    └── RegisterClassW()  // Registers "SBglWindowClass"
    └── CreateWindowExW() // Creates actual window instance

sbgl_os_DestroyWindow()
    └── DestroyWindow()   // Destroys window instance
    └── UnregisterClassW() // Unregisters the class (SBgl 2026-05-14+)
```

### Important Notes

* **Class Unregistration**: As of the 2026-05-14 update, SBgl properly unregisters the window class when the window is destroyed. This prevents resource leaks when creating multiple windows or during hot-reload scenarios.

* **Multiple Windows**: While SBgl's API supports multiple contexts, the current implementation uses a single window class. Creating multiple windows reuses the same class registration.

* **Custom Window Classes**: If you need to integrate SBgl into an existing Windows application with your own window class, you would need to modify the platform layer or use the Linux platform instead.

---

## Cursor Locking and Raw Input

### Overview

Windows provides two methods for tracking mouse movement:

1. **Absolute Positioning** (Standard): Uses `GetCursorPos()` / `SetCursorPos()`
2. **Raw Input** (High-Precision): Uses `RegisterRawInputDevices()` and `WM_INPUT`

SBgl uses **Raw Input** when the cursor is locked for high-precision camera controls.

### Cursor Modes

#### Normal Mode (`SBGL_MOUSE_MODE_NORMAL`)
* Cursor is visible and moves freely
* Standard absolute positioning for mouse deltas
* Suitable for UI interaction

#### Hidden Mode (`SBGL_MOUSE_MODE_HIDDEN`)
* Cursor is invisible but still moves freely
* Standard absolute positioning
* Useful for applications that render their own cursor

#### Captured Mode (`SBGL_MOUSE_MODE_CAPTURED`)
* Cursor is invisible and locked to window center
* **Raw Input API** provides high-precision deltas
* Automatic cursor re-centering prevents hitting screen boundaries
* Ideal for first-person camera controls

### Raw Input Implementation

When `sbgl_SetMouseMode(ctx, SBGL_MOUSE_MODE_CAPTURED)` is called:

1. **Registration**: SBgl registers for raw mouse input:
   ```c
   RAWINPUTDEVICE rid = {
       .usUsagePage = 0x01,    // Generic Desktop
       .usUsage = 0x02,        // Mouse
       .dwFlags = RIDEV_NOLEGACY | RIDEV_CAPTUREMOUSE,
       .hwndTarget = window->hwnd
   };
   RegisterRawInputDevices(&rid, 1, sizeof(rid));
   ```

2. **Data Collection**: Each mouse movement generates a `WM_INPUT` message with raw hardware deltas (`lLastX`, `lLastY`).

3. **Accumulation**: Deltas are accumulated in the window struct:
   ```c
   window->accumulatedDeltaX += raw.mouse.lLastX;
   window->accumulatedDeltaY += raw.mouse.lLastY;
   ```

4. **Per-Frame Reset**: Accumulators are reset each frame after updating `mouseDeltaX/Y`.

### Comparison with Linux

| Feature | Windows (Raw Input) | Linux (Wayland) |
|---------|---------------------|-----------------|
| API | `RegisterRawInputDevices()` | `zwp_relative_pointer_v1` |
| Precision | Hardware-level deltas | Hardware-level deltas |
| Screen Boundaries | Cursor re-centered | Compositor handles confinement |
| Requires Lock | Yes | Yes |

### Important Considerations

* **Raw Input Only When Locked**: Raw Input is only registered when the cursor is in `SBGL_MOUSE_MODE_CAPTURED`. In normal mode, standard absolute positioning is used to minimize overhead.

* **Cursor Re-centering**: To prevent the cursor from hitting screen edges (which would stop raw input deltas), SBgl re-centers the cursor each frame. This is done via `SetCursorPos()` after processing the raw deltas.

* **Cleanup**: When the window is destroyed or cursor mode changes to unlocked, SBgl automatically unregisters the raw input device and releases cursor clipping.

---

## DPI Awareness

### Overview

Windows 10+ supports **Per-Monitor DPI Awareness V2**, allowing applications to:

* Render crisply on high-DPI displays (125%, 150%, 200% scaling)
* Dynamically adjust when moved between monitors with different DPIs
* Receive proper physical pixel dimensions for Vulkan swapchain creation

### Implementation

SBgl automatically sets DPI awareness at window creation:

```c
SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
```

### DPI Change Handling

When the window is moved to a display with different DPI or the system scaling changes:

1. Windows sends `WM_DPICHANGED` message
2. SBgl receives the new recommended window rectangle
3. Window is resized to match new DPI scale
4. `window->resized` flag is set
5. Vulkan swapchain is recreated with new dimensions

### Important Notes

* **Windows 10+ Only**: `DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2` requires Windows 10 version 1703 (Creators Update) or later. SBgl targets Windows 10+ as specified in the implementation plan.

* **Physical vs Logical Pixels**: With DPI awareness enabled, `sbgl_os_GetWindowSize()` returns **physical pixels** (the actual framebuffer size), not logical DPI-scaled units. This ensures Vulkan renders at native resolution.

* **No Blurry Scaling**: Without DPI awareness, Windows would bitmap-scale your application, resulting in blurry rendering. SBgl's implementation ensures crisp rendering at all DPI scales.

---

## Console vs Window Subsystem

### Overview

Windows executables can be built for two subsystems:

1. **Console Subsystem** (`/SUBSYSTEM:CONSOLE`) - Default
2. **Window Subsystem** (`/SUBSYSTEM:WINDOWS`)

### Console Subsystem (Default)

**Characteristics:**
* Console window appears alongside your application window
* `stdout` and `stderr` are visible
* Useful for debugging with `printf()` or `fprintf(stderr, ...)`
* SBgl's error reporting (`OutputDebugStringW` + `stderr`) is visible

**Build Configuration:**
```cmake
# Default - no changes needed
add_executable(myapp main.c)
target_link_libraries(myapp PRIVATE sbgl)
```

**Entry Point:**
```c
int main(void) {
    // Standard entry point
    sbgl_InitResult res = sbgl_Init(800, 600, "My App");
    // ...
}
```

### Window Subsystem

**Characteristics:**
* No console window appears
* Cleaner presentation for end-user applications
* `stdout`/`stderr` are still accessible if redirected
* Use DebugView or Visual Studio Output window to see SBgl debug messages

**Build Configuration:**
```cmake
add_executable(myapp WIN32 main.c)  # Note: WIN32 keyword
target_link_libraries(myapp PRIVATE sbgl)
```

Or in CMakeLists.txt:
```cmake
set_property(TARGET myapp PROPERTY WIN32_EXECUTABLE TRUE)
```

**Entry Point:**
```c
#include <windows.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
                   LPSTR lpCmdLine, int nCmdShow) {
    sbgl_InitResult res = sbgl_Init(800, 600, "My App");
    // ...
    return 0;
}
```

### Recommendation

* **Development**: Use Console Subsystem for easy printf debugging
* **Production**: Use Window Subsystem for clean user experience

SBgl's dual error reporting (`OutputDebugStringW` + `stderr`) ensures you can still see errors in:
* Visual Studio Output window (Window Subsystem)
* Console window (Console Subsystem)
* DebugView utility from Microsoft (Window Subsystem)

---

## Error Reporting

### Overview

SBgl on Windows uses **dual error reporting** to ensure errors are visible in all contexts:

1. **`OutputDebugStringW()`**: Messages appear in Visual Studio Output window and DebugView
2. **`fprintf(stderr, ...)`**: Messages appear in console (if present)

### Implementation

```c
static void win32_report_error(const wchar_t* message) {
    // Visual Studio / DebugView
    OutputDebugStringW(message);
    OutputDebugStringW(L"\n");
    
    // Console (if available)
    int len = WideCharToMultiByte(CP_UTF8, 0, message, -1, NULL, 0, NULL, NULL);
    char* utf8 = malloc(len);
    WideCharToMultiByte(CP_UTF8, 0, message, -1, utf8, len, NULL, NULL);
    fprintf(stderr, "[Win32] %s\n", utf8);
    free(utf8);
}
```

### When Errors Are Reported

* Window creation failures
* Raw input registration failures
* Other critical Win32 API failures

### Viewing Errors

**Visual Studio (Window Subsystem):**
```
View → Output (or Ctrl+Alt+O)
```

**DebugView (Window Subsystem, standalone tool):**
Download from [Microsoft Sysinternals](https://docs.microsoft.com/en-us/sysinternals/downloads/debugview)

**Console (Console Subsystem):**
Errors appear directly in the console window.

---

## Key Repeat Behavior

### Overview

When a key is held down, Windows generates multiple `WM_KEYDOWN` messages (auto-repeat). SBgl filters these to match Linux/Wayland behavior.

### Implementation

```c
case WM_KEYDOWN: {
    // Bit 30 of lparam indicates previous key state
    bool isRepeat = (lparam & (1 << 30)) != 0;
    
    if (!isRepeat) {
        input->keysPressed[code] = true;  // Only set on initial press
    }
    input->keysDown[code] = true;         // Always set
}
```

### Behavior

| Action | `keysDown[code]` | `keysPressed[code]` |
|--------|------------------|---------------------|
| Initial key press | `true` | `true` |
| Key held (auto-repeat) | `true` | `false` (filtered) |
| Key release | `false` | `false` |

This matches the Wayland/X11 behavior and prevents unexpected repeated actions when holding a key.

---

## Scancode Mapping

### Overview

SBgl provides a platform-independent scancode system based on USB HID usage tables. On Windows, Virtual Keys (VK_*) are mapped to SBgl scancodes.

### Supported Keys

SBgl on Windows supports the full range of standard keys:

* **Alphanumeric**: A-Z, 0-9
* **Function Keys**: F1-F12
* **Navigation**: Arrow keys, HOME, END, PAGEUP, PAGEDOWN, INSERT, DELETE
* **Numpad**: 0-9, operators (+, -, *, /), ENTER, PERIOD
* **Modifiers**: LSHIFT, RSHIFT, LCTRL, RCTRL, LALT, RALT
* **Symbols**: Semicolon, Quote, Grave, Comma, Period, Slash, Backslash, Brackets
* **Lock Keys**: CAPSLOCK, NUMLOCK, SCROLLLOCK

### Legacy Modifier Handling

Windows provides both specific (VK_LSHIFT, VK_RSHIFT) and generic (VK_SHIFT) virtual keys. SBgl maps as follows:

* `VK_LSHIFT` → `SBGL_SCANCODE_LSHIFT`
* `VK_RSHIFT` → `SBGL_SCANCODE_RSHIFT`
* `VK_SHIFT` → `SBGL_SCANCODE_LSHIFT` (fallback)

This ensures backward compatibility with applications that only check for generic shift.

---

## Client Area Size

### Overview

In Win32 API, `CreateWindowEx()` parameters specify the **total window size** including:
* Title bar
* Border/frame
* Menu (if present)

This means requesting an 800x600 window would result in a smaller drawable area (e.g., 784x561 depending on Windows theme and DPI).

### SBgl Solution

SBgl uses `AdjustWindowRectEx()` to calculate the required window size:

```c
RECT rect = { 0, 0, width, height };  // Desired client area
AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, FALSE, 0);
// rect now contains total window size needed

CreateWindowExW(..., 
    rect.right - rect.left,  // Total width
    rect.bottom - rect.top,  // Total height
    ...
);
```

### Result

When you request an 800x600 window:
* **Client Area**: Exactly 800x600 pixels (drawable region)
* **Total Window**: Larger (e.g., 816x639) including borders/title bar
* **Vulkan Swapchain**: Created at 800x600 to match client area

This ensures the rendering resolution matches your expectations regardless of Windows theme, border styles, or DPI settings.

---

## Additional Resources

* [Microsoft Win32 API Documentation](https://docs.microsoft.com/en-us/windows/win32/apiindex/windows-api-list)
* [Raw Input Device Documentation](https://docs.microsoft.com/en-us/windows/win32/inputdev/raw-input)
* [High DPI Desktop Application Development](https://docs.microsoft.com/en-us/windows/win32/hidpi/high-dpi-desktop-application-development-on-windows)
* [Vulkan on Windows](https://vulkan-tutorial.com/Drawing_a_triangle/Presentation/Window_surface)

---

## Summary of Windows-Specific Behaviors

| Feature | Windows Implementation | Notes |
|---------|------------------------|-------|
| **Window Class** | Registered/unregistered per window | Prevents resource leaks |
| **Cursor Lock** | Raw Input API + centering | High-precision camera control |
| **DPI Handling** | Per-Monitor V2 awareness | Crisp rendering on all displays |
| **Error Output** | DebugString + stderr | Visible in VS and console |
| **Key Repeat** | Filtered using lparam bit 30 | Matches Linux behavior |
| **Window Size** | AdjustWindowRectEx | Accurate client area sizing |
| **Entry Point** | main() or WinMain() | Console or Window subsystem |

For platform-agnostic code, use the standard SBgl API. The behaviors documented here are implementation details that ensure correct operation on Windows.
