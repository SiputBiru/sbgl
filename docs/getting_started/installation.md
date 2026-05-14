# Installation Guide

This guide covers setting up the development environment and building SBgl for different platforms.

## Prerequisites

* **Compiler**: GCC 9+, Clang 10+, or MSVC 2019+.
* **Build System**: CMake 3.15+.
* **Graphics**: Vulkan SDK (Headers and `glslc` compiler required). Static linking to `vulkan-1` or `libvulkan` is **not** required as SBgl uses the `volk` meta-loader.

---

## Configuration Flags

SBgl provides several CMake options to customize the build:

| Flag | Description | Default |
| :--- | :--- | :--- |
| `SBGL_BUILD_EXAMPLES` | Compiles the example applications in `examples/`. | `OFF` |
| `SBGL_BUILD_TESTS` | Compiles the unit tests in `tests/`. | `OFF` |
| `SBGL_USE_WAYLAND` | (Linux Only) Uses native Wayland. If `OFF`, X11 is used. | `ON` |
| `CMAKE_BUILD_TYPE` | Set to `Debug` for development or `Release` for performance. | `Release` |

---

## Dependency Management

SBgl utilizes a hybrid dependency model to minimize manual setup:

* **Automatic (`FetchContent`)**: High-level dependencies like **volk** are automatically downloaded and configured during the CMake configuration phase.
* **External (System)**: Platform-specific libraries (Wayland, X11) and the Vulkan SDK must be present on the host system.
* **Internal (Embedded)**: Small, critical utilities (like `stb_perlin`) are embedded directly in the source tree.

---

## Installing on Linux (Wayland or X11)

### System Dependencies

Install the required development libraries for the preferred display protocol:

* **Wayland**: `libwayland-dev`, `wayland-protocols`.
* **X11**: `libx11-dev`.
* **Vulkan**: Vulkan SDK headers and `glslc`.

### Build Instructions

```bash
# Default build (Wayland enabled)
cmake -B build -DSBGL_BUILD_EXAMPLES=ON
cmake --build build

# X11 build
cmake -B build-x11 -DSBGL_USE_WAYLAND=OFF -DSBGL_BUILD_EXAMPLES=ON
cmake --build build-x11
```

---

## Installing on Windows (Native)

### Option 1: Visual Studio 2019/2022 (MSVC)

#### Prerequisites

* **Visual Studio 2019 or 2022** with the "Desktop development with C++" workload.
* **Vulkan SDK** (see [Vulkan SDK Installation](#vulkan-sdk-installation) section below).

#### Building from Visual Studio IDE

1. Open Visual Studio
2. Select **"Open a local folder"** (or File → Open → CMake...)
3. Navigate to the SBgl root directory and select it
4. Visual Studio will auto-detect the `CMakeLists.txt` and configure the project
5. Select build configuration (Debug or Release) from the toolbar
6. Build → Build All (or press **Ctrl+Shift+B**)
7. Run examples from the generated `build/examples/` directory

#### Building from Developer Command Prompt

Open **"Developer Command Prompt for VS 2019"** or **"Developer Command Prompt for VS 2022"** (available from Start Menu):

```powershell
# Navigate to SBgl directory
cd C:\path\to\sbgl

# Configure (Release is default, use -DCMAKE_BUILD_TYPE=Debug for debug builds)
cmake -B build -DSBGL_BUILD_EXAMPLES=ON -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build --config Release

# Run an example
.\build\examples\Release\hello_window.exe
```

### Option 2: MinGW-w64 (GCC)

#### Prerequisites

* **MinGW-w64 GCC 9+** (via MSYS2, standalone installer, or Chocolatey)
* **CMake 3.15+**
* **Ninja** (optional but recommended for faster builds)
* **Vulkan SDK** (see [Vulkan SDK Installation](#vulkan-sdk-installation) section below).

#### Using MSYS2 (Recommended)

1. Install MSYS2 from [https://www.msys2.org/](https://www.msys2.org/)
2. Open **"MSYS2 MinGW 64-bit"** terminal from Start Menu
3. Install required packages:

```bash
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja
```

4. Build SBgl:

```bash
cd /c/path/to/sbgl
cmake -B build -G Ninja -DSBGL_BUILD_EXAMPLES=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

#### Using Standalone MinGW

If you have MinGW-w64 installed separately (e.g., from mingw-w64.org or Chocolatey):

```powershell
# Ensure mingw32-make or ninja is in your PATH
# You can verify with: where mingw32-make

cmake -B build -G "MinGW Makefiles" -DSBGL_BUILD_EXAMPLES=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

#### Troubleshooting MinGW Builds

* **Error: "No CMAKE_C_COMPILER could be found"**: Ensure MinGW's `bin` directory is in your PATH environment variable.
* **Error: "Cannot find VULKAN_SDK"**: See [Vulkan SDK Installation](#vulkan-sdk-installation) section below.

---

## Vulkan SDK Installation

### Windows

1. **Download**: Get the Vulkan SDK from [https://vulkan.lunarg.com/sdk/home](https://vulkan.lunarg.com/sdk/home)
2. **Install**: Run the installer and follow the prompts
3. **Verify**: The installer should automatically set the `VULKAN_SDK` environment variable. Verify it:

```powershell
# In PowerShell
$env:VULKAN_SDK

# In Command Prompt
echo %VULKAN_SDK%
```

If not set, add it manually:
* **Variable name**: `VULKAN_SDK`
* **Variable value**: `C:\VulkanSDK\1.3.XXX.X` (replace with your version)

4. **Verify glslc is accessible**:

```powershell
where glslc
# Should output: C:\VulkanSDK\1.3.XXX.X\Bin\glslc.exe
```

### Linux

```bash
# Ubuntu/Debian
sudo apt install vulkan-tools vulkan-validationlayers-dev spirv-tools

# Arch Linux
sudo pacman -S vulkan-devel vulkan-tools

# Or download from https://vulkan.lunarg.com/sdk/home for the full SDK
```

---

## Troubleshooting

### Windows-Specific Issues

#### "Cannot find VULKAN_SDK" or "glslc not found"

**Cause**: Vulkan SDK not installed or environment variable not set.

**Solution**: 
1. Install Vulkan SDK (see [Vulkan SDK Installation](#vulkan-sdk-installation))
2. Restart your terminal/IDE after installation
3. Verify with `echo %VULKAN_SDK%`

#### "Cannot find compiler" (MSVC)

**Cause**: Not running from Developer Command Prompt or Visual Studio environment.

**Solution**: Use "Developer Command Prompt for VS 2019/2022" from Start Menu, or open the project in Visual Studio IDE.

#### "LNK2019: unresolved external symbol" with MinGW

**Cause**: Name mangling differences between C and C++ when linking.

**Solution**: Ensure SBgl headers are included with proper `extern "C"` guard in C++ projects:

```cpp
extern "C" {
#include "sbgl.h"
}
```

#### "The application was unable to start correctly (0xc000007b)"

**Cause**: Mixing 32-bit and 64-bit components (compiler, Vulkan SDK, or SBgl build).

**Solution**: Ensure all components are 64-bit:
* Use "x64" or "x86_64" versions of compilers
* Install 64-bit Vulkan SDK
* Build with `-DCMAKE_GENERATOR_PLATFORM=x64` if needed

#### "The code execution cannot proceed because vulkan-1.dll was not found"

**Cause**: Vulkan runtime not in PATH or not installed.

**Solution**: 
* Install Vulkan SDK (includes runtime)
* Or download just the runtime from [LunarG](https://vulkan.lunarg.com/sdk/home)

### Linux-Specific Issues

#### "wayland-client.h not found"

**Cause**: Wayland development headers not installed.

**Solution**: `sudo apt install libwayland-dev wayland-protocols`

#### "X11/Xlib.h not found"

**Cause**: X11 development headers not installed.

**Solution**: `sudo apt install libx11-dev`

### General Issues

#### "CMake Error: Could not find CMAKE_ROOT"

**Cause**: CMake not properly installed or not in PATH.

**Solution**: Install CMake 3.15+ and ensure it's in your system PATH.

#### Examples build but shaders fail to compile

**Cause**: `glslc` (Vulkan shader compiler) not found.

**Solution**: Install Vulkan SDK and ensure `VULKAN_SDK` environment variable points to the SDK root.

---

## Cross-Compilation (Linux to Windows)

SBgl supports building Windows binaries from a Linux host using the MinGW-w64 toolchain.

### Prerequisites

Install the MinGW-w64 compiler:

* **Debian/Ubuntu**: `sudo apt install mingw-w64`
* **Arch**: `sudo pacman -S mingw-w64-gcc`

### Build Instructions

Specify the Windows system name and the MinGW compiler to CMake:

```bash
cmake -B build-win \
  -DCMAKE_SYSTEM_NAME=Windows \
  -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
  -DSBGL_BUILD_EXAMPLES=ON

cmake --build build-win
```

---

## Integration via FetchContent

The simplest way to use SBgl in an external project is via CMake's `FetchContent`:

```cmake
include(FetchContent)
FetchContent_Declare(
    sbgl
    GIT_REPOSITORY https://github.com/SiputBiru/sbgl.git
    GIT_TAG main
)
FetchContent_MakeAvailable(sbgl)

target_link_libraries(target_application PRIVATE sbgl)
```
