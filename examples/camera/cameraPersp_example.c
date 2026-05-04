#include "sbgl_math.h"
#include "sbgl_camera.h"
#include <stdio.h>

int main(void) {
    printf("--- SBgl Perspective Camera & Batch Collision Example ---\n\n");

    // Initialize a perspective camera
    float aspect = 16.0f / 9.0f;
    sbgl_Camera cam = sbgl_CameraPerspective(45.0f * (SBGL_PI / 180.0f), aspect, 0.1f, 100.0f);
    
    // Move the camera
    cam.position = sbgl_Vec3Set(0, 2, 10);
    cam.target = sbgl_Vec3Set(0, 0, 0);

    // Get matrices
    sbgl_Mat4 view = sbgl_CameraGetView(&cam);
    sbgl_Mat4 proj = sbgl_CameraGetProjection(&cam);

    printf("Camera Position: (%.2f, %.2f, %.2f)\n", cam.position.x, cam.position.y, cam.position.z);
    printf("View Matrix (first column): [%.2f, %.2f, %.2f, %.2f]\n", view.m[0][0], view.m[0][1], view.m[0][2], view.m[0][3]);
    printf("Projection Matrix (first column): [%.2f, %.2f, %.2f, %.2f]\n\n", proj.m[0][0], proj.m[0][1], proj.m[0][2], proj.m[0][3]);

    // Batch Collision Setup
    sbgl_Ray ray;
    ray.origin = sbgl_Vec3Set(0, 2, 10);
    ray.direction = sbgl_Vec3Normalize(sbgl_Vec3Sub(sbgl_Vec3Set(0, 0, 0), ray.origin));

    sbgl_AABB boxes[3];
    // Box 1: At the origin (Should hit)
    boxes[0].min = sbgl_Vec3Set(-1, -1, -1);
    boxes[0].max = sbgl_Vec3Set(1, 1, 1);
    // Box 2: Off to the side (Should miss)
    boxes[1].min = sbgl_Vec3Set(5, 0, 0);
    boxes[1].max = sbgl_Vec3Set(7, 2, 2);
    // Box 3: Behind the camera (Should miss)
    boxes[2].min = sbgl_Vec3Set(0, 2, 15);
    boxes[2].max = sbgl_Vec3Set(1, 3, 16);

    sbgl_HitResult results[3];
    sbgl_RayAABBIntersectBatch(ray, boxes, results, 3);

    printf("Batch Ray-AABB Results:\n");
    for (int i = 0; i < 3; ++i) {
        if (results[i].hit) {
            printf("Box %d: HIT at distance %.2f, normal (%.1f, %.1f, %.1f)\n", 
                   i, results[i].distance, results[i].normal.x, results[i].normal.y, results[i].normal.z);
        } else {
            printf("Box %d: MISS\n", i);
        }
    }

    return 0;
}
