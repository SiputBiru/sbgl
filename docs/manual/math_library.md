# Math Library Architecture

The math library provides Data-Oriented Design (DOD) workflows. The library is contained within a single header file (`sbgl_math.h`) and utilizes `static inline` functions for compiler inlining.

## Architecture and Design

*   **SIMD-Ready Layouts**: Primary types (`sbgl_Vec3`, `sbgl_Vec4`, `sbgl_Mat4`, `sbgl_Quat`) are 16-byte aligned. This allows the compiler to utilize SSE/AVX instructions when processing arrays of math types.
*   **Padded Vectors**: `sbgl_Vec3` is explicitly padded to 16 bytes. While this increases memory footprint by 4 bytes per vector, it ensures that 3D data is 128-bit aligned. This is a deliberate trade-off to enable SIMD loads, ensure cache line alignment in arrays, and maintain direct compatibility with GPU buffer layouts (std140/std430).
*   **Data Access**: Types are implemented as `unions`, providing access to individual components (`x, y, z, w`) or raw arrays (`v[4]`).
*   **Inverse Square Root**: The library includes an implementation of the Fast Inverse Square Root algorithm.

## Primary Data Types

| Type | Alignment | Description |
| :--- | :--- | :--- |
| sbgl_Vec2 | Default | 2D vector (float). |
| sbgl_Vec3 | 16-byte | 3D vector, padded to 16 bytes. |
| sbgl_Vec4 | 16-byte | 4D vector. |
| sbgl_Quat | 16-byte | Quaternion. |
| sbgl_Mat4 | 16-byte | 4x4 matrix, column-major. |

## Library Capabilities

### Inverse Square Root
*   `sbgl_InvSqrt()`: Computes `1.0f / sqrtf(x)` using bit-level manipulation and two Newton-Raphson iterations. This implementation is inspired by the [Fast Inverse Square Root](https://en.wikipedia.org/wiki/Fast_inverse_square_root) algorithm historically used in Quake III Arena.

### Constructors
The library provides `static inline` constructor functions to initialize types and ensure correct padding.
*   `sbgl_Vec2Set()`, `sbgl_Vec3Set()`, `sbgl_Vec4Set()`, `sbgl_QuatSet()`.

### Vector Operations
*   **Arithmetic**: `sbgl_Vec3Add()`, `sbgl_Vec3Sub()`, `sbgl_Vec3Mul()`.
*   **Geometry**: `sbgl_Vec3Dot()`, `sbgl_Vec3Cross()`, `sbgl_Vec3Length()`, `sbgl_Vec3Normalize()`.

### Matrix Transformations
*   **Generators**: `sbgl_Mat4Identity()`, `sbgl_Mat4Perspective()`, `sbgl_Mat4Orthographic()`, `sbgl_Mat4LookAt()`.
*   **Affine Transformations**: `sbgl_Mat4Translate()`, `sbgl_Mat4Rotate()`, `sbgl_Mat4Scale()`.
*   **Multiplication**: `sbgl_Mat4Mul()`, `sbgl_Mat4MulVec4()`.

### Quaternion Operations
*   **Operations**: `sbgl_QuatIdentity()`, `sbgl_QuatMul()`, `sbgl_QuatFromAxisAngle()`.
*   **Conversion**: `sbgl_QuatToMat4()`.

## Data Initialization

The library utilizes a double-brace syntax `{{ ... }}` for vector and quaternion initialization. This is required due to the nested nature of the `union` and anonymous `struct` definitions used to provide component-wise access.

### Technical Detail
Types like `sbgl_Vec3` are defined as a `union` where the first member is an anonymous `struct`. 

```c
typedef union {
    struct { float x, y, z; float _pad; };
    float v[4];
} sbgl_Vec3;
```

When initializing these types with compound literals:
1.  The **outer braces** `{}` initialize the `union`.
2.  The **inner braces** `{}` initialize the anonymous `struct` (the first member of the union).

Using single braces may result in compiler warnings (e.g., `-Wmissing-braces`) regarding the initialization of subobjects.

## Implementation Examples

### Vector Operations
```c
sbgl_Vec3 position = sbgl_Vec3Set(1.0f, 2.0f, 3.0f);
sbgl_Vec3 velocity = sbgl_Vec3Set(0.0f, 1.0f, 0.0f);

// Vector addition
sbgl_Vec3 next_pos = sbgl_Vec3Add(position, velocity);
```

### Transformation Matrices
```c
// Create a translation matrix
sbgl_Mat4 model = sbgl_Mat4Translate(sbgl_Vec3Set(10.0f, 0.0f, 0.0f));

// Create a perspective projection
float fov = SBGL_PI / 4.0f; // 45 degrees
float aspect = 800.0f / 600.0f;
sbgl_Mat4 projection = sbgl_Mat4Perspective(fov, aspect, 0.1f, 100.0f);

// Matrix multiplication (MVP)
sbgl_Mat4 view = sbgl_Mat4Identity();
sbgl_Mat4 mvp = sbgl_Mat4Mul(projection, sbgl_Mat4Mul(view, model));
```

### Rotation with Quaternions
```c
// Define 90-degree rotation around the Y axis
sbgl_Quat rotation = sbgl_QuatFromAxisAngle(sbgl_Vec3Set(0.0f, 1.0f, 0.0f), SBGL_PI / 2.0f);

// Convert to matrix for use in the rendering pipeline
sbgl_Mat4 rotation_matrix = sbgl_QuatToMat4(rotation);
```

## Internal Implementation

The library provides `static inline` definitions, allowing the compiler to optimize math operations directly into the calling code. The aligned memory layout is intended to facilitate data streaming into SIMD registers during batch processing.
