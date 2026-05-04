#include <stdio.h>
#include <assert.h>
#include <math.h>
#include "../include/sbgl_math.h"

static void test_inv_sqrt(void) {
    float val = 4.0f;
    float expected = 0.5f;
    float result = sbgl_InvSqrt(val);
    printf("InvSqrt(4.0): Expected %f, Got %f\n", expected, result);
    assert(fabsf(result - expected) < 0.001f);
}

static void test_vec3(void) {
    sbgl_Vec3 a = {{ 1.0f, 0.0f, 0.0f, 0 }};
    sbgl_Vec3 b = {{ 0.0f, 1.0f, 0.0f, 0 }};
    
    // Dot product
    float dot = sbgl_Vec3Dot(a, b);
    printf("Vec3 Dot: Expected 0.0, Got %f\n", dot);
    assert(dot == 0.0f);

    // Cross product
    sbgl_Vec3 cross = sbgl_Vec3Cross(a, b);
    printf("Vec3 Cross: Got {%f, %f, %f}\n", cross.x, cross.y, cross.z);
    assert(cross.x == 0.0f && cross.y == 0.0f && cross.z == 1.0f);

    // Normalize
    sbgl_Vec3 c = {{ 5.0f, 0.0f, 0.0f, 0 }};
    sbgl_Vec3 norm = sbgl_Vec3Normalize(c);
    printf("Vec3 Normalize: Got {%f, %f, %f}\n", norm.x, norm.y, norm.z);
    assert(fabsf(norm.x - 1.0f) < 0.001f);
}

static void test_mat4(void) {
    sbgl_Mat4 identity = sbgl_Mat4Identity();
    assert(identity.m[0][0] == 1.0f && identity.m[3][3] == 1.0f);

    sbgl_Vec3 trans_vec = {{ 10.0f, 20.0f, 30.0f, 0 }};
    sbgl_Mat4 translation = sbgl_Mat4Translate(trans_vec);
    
    sbgl_Vec4 point = {{ 0.0f, 0.0f, 0.0f, 1.0f }};
    sbgl_Vec4 result = sbgl_Mat4MulVec4(translation, point);
    
    printf("Mat4 Translate Point: Got {%f, %f, %f, %f}\n", result.x, result.y, result.z, result.w);
    assert(result.x == 10.0f && result.y == 20.0f && result.z == 30.0f && result.w == 1.0f);
}

static void test_quat(void) {
    // 90 degrees around Y axis
    sbgl_Vec3 axis = {{ 0.0f, 1.0f, 0.0f, 0 }};
    sbgl_Quat q = sbgl_QuatFromAxisAngle(axis, SBGL_PI / 2.0f);
    
    sbgl_Mat4 rot_mat = sbgl_QuatToMat4(q);
    
    // Rotate point (1, 0, 0) should become (0, 0, -1) in right-handed system
    sbgl_Vec4 p = {{ 1.0f, 0.0f, 0.0f, 1.0f }};
    sbgl_Vec4 res = sbgl_Mat4MulVec4(rot_mat, p);
    
    printf("Quat Rotate (90 deg Y): Got {%f, %f, %f}\n", res.x, res.y, res.z);
    assert(fabsf(res.x - 0.0f) < 0.001f);
    assert(fabsf(res.z - (-1.0f)) < 0.001f);
}

int main(void) {
    printf("--- SBgl Math Test ---\n");
    test_inv_sqrt();
    test_vec3();
    test_mat4();
    test_quat();
    printf("All math tests passed!\n");
    return 0;
}
