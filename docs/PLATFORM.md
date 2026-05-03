# SBgl Platform Abstraction Layer (HAL)

SBgl utilizes a strict Platform Abstraction Layer (HAL) to ensure that the core engine and user code remain completely isolated from OS-specific dependencies. This prevents heavy, namespace-polluting headers like `<windows.h>` or `<X11/Xlib.h>` from leaking into the user's application.

---

## The Interface (`sbgl_platform.h`)

The core interacts with the operating system through a set of explicitly defined functions in `src/core/sbgl_platform.h`. The interface mandates that all OS-specific state be kept opaque from the rest of the engine.

### Relationship with `sbgl_Context`

The public `sbgl_Context` structure serves as a thin shell. Its `inner` member points to a private `sbgl_InternalContext`, which encapsulates the platform layer state:

- **Context Shell:** The user holds a `sbgl_Context*`.
- **Internal State:** This contains the `sbgl_InternalContext`, which manages engine-wide state (Arena, clear colors, etc.).
- **Platform Handle:** The `sbgl_InternalContext` holds a pointer to an opaque `sbgl_Window`, which is defined only within the specific platform's source files (e.g., `window_wayland.c`).

This hierarchy allows the engine to pass a generic `sbgl_Context` to the public API while the platform layer internally retrieves the specific handles it needs to communicate with the OS.

---

## Core HAL Interface

The most critical functions in the HAL manage the lifecycle and event loop of the native window:

```c
// Creates a native OS window and allocates its state from the provided arena
sbgl_Window* sbgl_os_CreateWindow(struct SblArena* arena, sbgl_InputState* input, int width, int height, const char* title);

// Dispatches OS events (e.g., messages, signals, protocol requests)
void sbgl_os_PollEvents(sbgl_Window* window);
```

---

## Window Lifecycle and Closure

The `sbgl_WindowShouldClose` function provides a mechanism for the engine to report that the operating system or the user has requested the window to be closed. It does **not** automatically terminate the application; instead, it sets a flag that the user should poll in their main loop.

### Platform-Specific Triggers

The internal `shouldClose` flag is set by the platform layer in response to specific OS events:

*   **Win32:** Triggered when the `WindowProc` intercepts a `WM_CLOSE` message. This typically occurs when the user clicks the "X" button or presses `Alt+F4`.
*   **Wayland:** Triggered by the `xdg_toplevel_listener`'s `close` event. This is sent by the compositor when it requests the surface to be destroyed.
*   **X11:** Triggered when an `XEvent` of type `ClientMessage` is received containing the `WM_DELETE_WINDOW` atom.

### User Responsibility

Once `sbgl_WindowShouldClose` returns `true`, it is the user's responsibility to:
1.  Exit the main execution loop.
2.  Call `sbgl_Shutdown(ctx)` to perform a graceful cleanup of all native handles and memory arenas.

---

Each platform backend implements its own concrete version of `struct sbgl_Window` to bridge the gap between SBgl's Data-Oriented arena allocator and the OS's native requirements.

### Windows (Win32)

The Win32 implementation registers a `WNDCLASSW` and creates an `HWND`. It utilizes `GWLP_USERDATA` to associate the `sbgl_Window` state with the native window procedure.

```c
struct sbgl_Window {
    HWND hwnd;
    bool shouldClose;
    bool resized;
    int width, height;
    sbgl_InputState* input;
};

sbgl_Window* sbgl_os_CreateWindow(struct SblArena* arena, sbgl_InputState* input, int width, int height, const char* title) {
    // Register Window Class
    WNDCLASSW wc = {0};
    wc.lpfnWndProc   = WindowProc; // Handles WM_SIZE, WM_CLOSE, etc.
    wc.hInstance     = GetModuleHandle(NULL);
    wc.lpszClassName = L"SBglWindowClass";
    RegisterClassW(&wc);

    // Create Native Window
    HWND hwnd = CreateWindowExW(
        0, wc.lpszClassName, L"SBgl",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height,
        NULL, NULL, wc.hInstance, NULL
    );

    // Allocate Opaque State via Arena
    sbgl_Window* window = SBL_ARENA_PUSH_STRUCT(arena, sbgl_Window);
    window->hwnd = hwnd;
    window->input = input;
    
    // Link State to HWND for the WindowProc to retrieve
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)window);
    ShowWindow(hwnd, SW_SHOW);

    return window;
}
```

### Linux (Wayland)

The Wayland implementation is asynchronous and uses the XDG-Shell protocol. It manages the connection to the `wl_display` and handles surface state transitions via listeners.

```c
struct sbgl_Window {
    struct wl_surface* surface;     
    struct xdg_surface* xdg_surface; 
    struct xdg_toplevel* xdg_toplevel;
    sbgl_InputState* input;
    bool shouldClose;
};

sbgl_Window* sbgl_os_CreateWindow(struct SblArena* arena, sbgl_InputState* input, int width, int height, const char* title) {
    // Connect and initialize globals (Registry/Compositor)
    g_display = wl_display_connect(NULL);
    
    // Allocate Opaque State via Arena
    sbgl_Window* window = SBL_ARENA_PUSH_STRUCT(arena, sbgl_Window);
    window->input = input;

    // Create Wayland protocol objects
    window->surface = wl_compositor_create_surface(g_compositor);
    window->xdg_surface = xdg_wm_base_get_xdg_surface(g_wm_base, window->surface);
    window->xdg_toplevel = xdg_surface_get_toplevel(window->xdg_surface);
    
    xdg_toplevel_set_title(window->xdg_toplevel, title);

    // Commit to notify the compositor
    wl_surface_commit(window->surface);
    wl_display_roundtrip(g_display);
    
    return window;
}
```

### Linux (X11)

The X11 implementation uses `XCreateSimpleWindow` and interacts with the Window Manager via Atoms to handle graceful termination.

```c
struct sbgl_Window {
    Window window;
    Atom wmDeleteMessage;
    sbgl_InputState* input;
    bool shouldClose;
};

sbgl_Window* sbgl_os_CreateWindow(struct SblArena* arena, sbgl_InputState* input, int width, int height, const char* title) {
    // Open connection to X Server
    g_x11_display = XOpenDisplay(NULL);
    int screen = DefaultScreen(g_x11_display);
    
    // Create Native Window
    Window win = XCreateSimpleWindow(
        g_x11_display, RootWindow(g_x11_display, screen),
        0, 0, width, height, 0,
        BlackPixel(g_x11_display, screen), WhitePixel(g_x11_display, screen)
    );

    // Set Window Manager protocols (e.g., close button support)
    Atom wmDeleteMessage = XInternAtom(g_x11_display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(g_x11_display, win, &wmDeleteMessage, 1);
    XMapWindow(g_x11_display, win);

    // Allocate Opaque State via Arena
    sbgl_Window* window = SBL_ARENA_PUSH_STRUCT(arena, sbgl_Window);
    window->window = win;
    window->wmDeleteMessage = wmDeleteMessage;
    window->input = input;

    return window;
}
```

---

## References & Further Reading

For detailed technical specifications of the underlying platform protocols and APIs, refer to the following official documentation:

- **Win32 API:** [Windows Desktop App Development (MSDN)](https://learn.microsoft.com/en-us/windows/win32/desktop-programming)
- **Wayland Protocol:** [Wayland Explorer & Documentation](https://wayland.freedesktop.org/docs/html/)
- **X11 (Xlib):** [X.org Foundation Documentation](https://www.x.org/wiki/Development/)
- **Vulkan Surface Integration:** [Vulkan Spec - Window System Integration (WSI)](https://registry.khronos.org/vulkan/specs/1.3-extensions/html/vkspec.html#wsi)
