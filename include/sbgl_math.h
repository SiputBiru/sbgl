/**
 * @file sbgl_math.h
 * @brief Single-header math library for SBgl.
 * 
 * Provides SIMD-ready vector, matrix, and quaternion operations using 
 * aligned memory layouts and static inline functions.
 */

#ifndef SBGL_MATH_H
#define SBGL_MATH_H

#include <math.h>
#include <stdint.h>
#include <stdbool.h>

#if defined(_MSC_VER)
    #define SBGL_ALIGN(n) __declspec(align(n))
#else
    #define SBGL_ALIGN(n) __attribute__((aligned(n)))
#endif

#define SBGL_PI 3.14159265358979323846f

// --- Types ---

/**
 * @brief 2D Vector.
 */
typedef union {
    struct { float x, y; };
    float v[2];
} sbgl_Vec2;

/**
 * @brief 3D Vector, 16-byte aligned and padded for SIMD safety.
 */
typedef union {
    struct { float x, y, z; float _pad; };
    float v[4];
} SBGL_ALIGN(16) sbgl_Vec3;

/**
 * @brief 4D Vector, 16-byte aligned.
 */
typedef union {
    struct { float x, y, z, w; };
    float v[4];
} SBGL_ALIGN(16) sbgl_Vec4;

/**
 * @brief Quaternion, 16-byte aligned.
 */
typedef union {
    struct { float x, y, z, w; };
    float v[4];
} SBGL_ALIGN(16) sbgl_Quat;

/**
 * @brief 4x4 Matrix, 16-byte aligned, column-major.
 */
typedef struct {
    SBGL_ALIGN(16) float m[4][4];
} sbgl_Mat4;

// --- Constructors ---

/** @brief Creates a Vec2. */
static inline sbgl_Vec2 sbgl_vec2(float x, float y) {
    return (sbgl_Vec2){{ x, y }};
}

/** @brief Creates a Vec3, correctly padded. */
static inline sbgl_Vec3 sbgl_vec3(float x, float y, float z) {
    return (sbgl_Vec3){{ x, y, z, 0.0f }};
}

/** @brief Creates a Vec4. */
static inline sbgl_Vec4 sbgl_vec4(float x, float y, float z, float w) {
    return (sbgl_Vec4){{ x, y, z, w }};
}

/** @brief Creates a Quat. */
static inline sbgl_Quat sbgl_quat(float x, float y, float z, float w) {
    return (sbgl_Quat){{ x, y, z, w }};
}

// --- Fast Math ---

/**
 * @brief John Carmack's Fast Inverse Square Root.
 * 
 * Uses C99-compliant union punning to avoid strict-aliasing violations.
 */
static inline float sbgl_InvSqrt(float x) {
    union {
        float f;
        uint32_t i;
    } pun;

    float xhalf = 0.5f * x;
    pun.f = x;
    pun.i = 0x5f3759df - (pun.i >> 1);
    pun.f = pun.f * (1.5f - xhalf * pun.f * pun.f);
    // Second iteration for better precision
    pun.f = pun.f * (1.5f - xhalf * pun.f * pun.f); 
    return pun.f;
}

// --- Vector Operations ---

/** @brief Adds two Vec3 vectors. */
static inline sbgl_Vec3 sbgl_Vec3Add(sbgl_Vec3 a, sbgl_Vec3 b) {
    return (sbgl_Vec3){{ a.x + b.x, a.y + b.y, a.z + b.z, 0.0f }};
}

/** @brief Subtracts b from a. */
static inline sbgl_Vec3 sbgl_Vec3Sub(sbgl_Vec3 a, sbgl_Vec3 b) {
    return (sbgl_Vec3){{ a.x - b.x, a.y - b.y, a.z - b.z, 0.0f }};
}

/** @brief Multiplies a Vec3 by a scalar. */
static inline sbgl_Vec3 sbgl_Vec3Mul(sbgl_Vec3 a, float s) {
    return (sbgl_Vec3){{ a.x * s, a.y * s, a.z * s, 0.0f }};
}

/** @brief Computes the dot product of two Vec3 vectors. */
static inline float sbgl_Vec3Dot(sbgl_Vec3 a, sbgl_Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

/** @brief Computes the cross product of two Vec3 vectors. */
static inline sbgl_Vec3 sbgl_Vec3Cross(sbgl_Vec3 a, sbgl_Vec3 b) {
    return (sbgl_Vec3){{
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x,
        0.0f
    }};
}

/** @brief Returns the length of a Vec3. */
static inline float sbgl_Vec3Length(sbgl_Vec3 v) {
    return sqrtf(sbgl_Vec3Dot(v, v));
}

/** @brief Normalizes a Vec3. */
static inline sbgl_Vec3 sbgl_Vec3Normalize(sbgl_Vec3 v) {
    float len_sq = sbgl_Vec3Dot(v, v);
    if (len_sq < 0.000001f) return (sbgl_Vec3){{0}};
    return sbgl_Vec3Mul(v, sbgl_InvSqrt(len_sq));
}

/** @brief Adds two Vec4 vectors. */
static inline sbgl_Vec4 sbgl_Vec4Add(sbgl_Vec4 a, sbgl_Vec4 b) {
    return (sbgl_Vec4){{ a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w }};
}

/** @brief Multiplies a Vec4 by a scalar. */
static inline sbgl_Vec4 sbgl_Vec4Mul(sbgl_Vec4 a, float s) {
    return (sbgl_Vec4){{ a.x * s, a.y * s, a.z * s, a.w * s }};
}

// --- Quaternion Operations ---

/** @brief Returns an identity quaternion. */
static inline sbgl_Quat sbgl_QuatIdentity(void) {
    return (sbgl_Quat){{ 0, 0, 0, 1.0f }};
}

/** @brief Multiplies two quaternions. */
static inline sbgl_Quat sbgl_QuatMul(sbgl_Quat a, sbgl_Quat b) {
    return (sbgl_Quat){{
        a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
        a.w * b.y + a.y * b.w + a.z * b.x - a.x * b.z,
        a.w * b.z + a.z * b.w + a.x * b.y - a.y * b.x,
        a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z
    }};
}

/** @brief Creates a quaternion from an axis and an angle (in radians). */
static inline sbgl_Quat sbgl_QuatFromAxisAngle(sbgl_Vec3 axis, float angle_rad) {
    float s = sinf(angle_rad / 2.0f);
    sbgl_Vec3 n = sbgl_Vec3Normalize(axis);
    return (sbgl_Quat){{
        n.x * s,
        n.y * s,
        n.z * s,
        cosf(angle_rad / 2.0f)
    }};
}

/** @brief Returns an identity matrix. */
static inline sbgl_Mat4 sbgl_Mat4Identity(void) {
    sbgl_Mat4 res = {0};
    res.m[0][0] = 1.0f;
    res.m[1][1] = 1.0f;
    res.m[2][2] = 1.0f;
    res.m[3][3] = 1.0f;
    return res;
}

/** @brief Converts a quaternion to a rotation matrix. */
static inline sbgl_Mat4 sbgl_QuatToMat4(sbgl_Quat q) {
    sbgl_Mat4 res = sbgl_Mat4Identity();
    float xx = q.x * q.x; float yy = q.y * q.y; float zz = q.z * q.z;
    float xy = q.x * q.y; float xz = q.x * q.z; float yz = q.y * q.z;
    float wx = q.w * q.x; float wy = q.w * q.y; float wz = q.w * q.z;

    res.m[0][0] = 1.0f - 2.0f * (yy + zz);
    res.m[0][1] = 2.0f * (xy + wz);
    res.m[0][2] = 2.0f * (xz - wy);

    res.m[1][0] = 2.0f * (xy - wz);
    res.m[1][1] = 1.0f - 2.0f * (xx + zz);
    res.m[1][2] = 2.0f * (yz + wx);

    res.m[2][0] = 2.0f * (xz + wy);
    res.m[2][1] = 2.0f * (yz - wx);
    res.m[2][2] = 1.0f - 2.0f * (xx + yy);

    return res;
}

// --- Matrix Operations ---

/** @brief Multiplies two matrices. */
static inline sbgl_Mat4 sbgl_Mat4Mul(sbgl_Mat4 a, sbgl_Mat4 b) {
    sbgl_Mat4 res = {0};
    for (int c = 0; c < 4; ++c) {
        for (int r = 0; r < 4; ++r) {
            res.m[c][r] = a.m[0][r] * b.m[c][0] +
                          a.m[1][r] * b.m[c][1] +
                          a.m[2][r] * b.m[c][2] +
                          a.m[3][r] * b.m[c][3];
        }
    }
    return res;
}

/** @brief Multiplies a matrix by a Vec4. */
static inline sbgl_Vec4 sbgl_Mat4MulVec4(sbgl_Mat4 m, sbgl_Vec4 v) {
    return (sbgl_Vec4){{
        m.m[0][0] * v.x + m.m[1][0] * v.y + m.m[2][0] * v.z + m.m[3][0] * v.w,
        m.m[0][1] * v.x + m.m[1][1] * v.y + m.m[2][1] * v.z + m.m[3][1] * v.w,
        m.m[0][2] * v.x + m.m[1][2] * v.y + m.m[2][2] * v.z + m.m[3][2] * v.w,
        m.m[0][3] * v.x + m.m[1][3] * v.y + m.m[2][3] * v.z + m.m[3][3] * v.w
    }};
}

/** @brief Creates a translation matrix. */
static inline sbgl_Mat4 sbgl_Mat4Translate(sbgl_Vec3 v) {
    sbgl_Mat4 res = sbgl_Mat4Identity();
    res.m[3][0] = v.x;
    res.m[3][1] = v.y;
    res.m[3][2] = v.z;
    return res;
}

/** @brief Creates a scaling matrix. */
static inline sbgl_Mat4 sbgl_Mat4Scale(sbgl_Vec3 v) {
    sbgl_Mat4 res = sbgl_Mat4Identity();
    res.m[0][0] = v.x;
    res.m[1][1] = v.y;
    res.m[2][2] = v.z;
    return res;
}

/** @brief Creates a perspective projection matrix. */
static inline sbgl_Mat4 sbgl_Mat4Perspective(float fov_y_rad, float aspect, float near, float far) {
    float f = 1.0f / tanf(fov_y_rad / 2.0f);
    sbgl_Mat4 res = {0};
    res.m[0][0] = f / aspect;
    res.m[1][1] = f;
    res.m[2][2] = far / (near - far);
    res.m[2][3] = -1.0f;
    res.m[3][2] = (far * near) / (near - far);
    return res;
}

/** @brief Creates a rotation matrix from axis and angle. */
static inline sbgl_Mat4 sbgl_Mat4Rotate(float angle_rad, sbgl_Vec3 axis) {
    sbgl_Quat q = sbgl_QuatFromAxisAngle(axis, angle_rad);
    return sbgl_QuatToMat4(q);
}

/** @brief Creates an orthographic projection matrix. */
static inline sbgl_Mat4 sbgl_Mat4Orthographic(float left, float right, float bottom, float top, float near, float far) {
    sbgl_Mat4 res = sbgl_Mat4Identity();
    res.m[0][0] = 2.0f / (right - left);
    res.m[1][1] = 2.0f / (top - bottom);
    res.m[2][2] = -2.0f / (far - near);
    res.m[3][0] = -(right + left) / (right - left);
    res.m[3][1] = -(top + bottom) / (top - bottom);
    res.m[3][2] = -(far + near) / (far - near);
    return res;
}

/** @brief Creates a look-at view matrix. */
static inline sbgl_Mat4 sbgl_Mat4LookAt(sbgl_Vec3 eye, sbgl_Vec3 center, sbgl_Vec3 up) {
    sbgl_Vec3 f = sbgl_Vec3Normalize(sbgl_Vec3Sub(center, eye));
    sbgl_Vec3 s = sbgl_Vec3Normalize(sbgl_Vec3Cross(f, up));
    sbgl_Vec3 u = sbgl_Vec3Cross(s, f);

    sbgl_Mat4 res = sbgl_Mat4Identity();
    res.m[0][0] = s.x;
    res.m[1][0] = s.y;
    res.m[2][0] = s.z;
    res.m[0][1] = u.x;
    res.m[1][1] = u.y;
    res.m[2][1] = u.z;
    res.m[0][2] = -f.x;
    res.m[1][2] = -f.y;
    res.m[2][2] = -f.z;
    res.m[3][0] = -sbgl_Vec3Dot(s, eye);
    res.m[3][1] = -sbgl_Vec3Dot(u, eye);
    res.m[3][2] = sbgl_Vec3Dot(f, eye);
    return res;
}

#endif // SBGL_MATH_H
