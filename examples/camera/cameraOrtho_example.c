#include "sbgl_math.h"
#include "sbgl_camera.h"
#include <stdio.h>

int main() {
    printf("--- SBgl Orthographic Camera & Batch Collision Example ---\n\n");

    // Initialize an orthographic camera for a 1280x720 viewport
    sbgl_OrthoParams params = { .left = 0, .right = 1280, .bottom = 720, .top = 0, .near_p = -1.0f, .far_p = 1.0f };
    sbgl_Camera cam = sbgl_CameraOrthographic(params);
    
    // Get matrices
    sbgl_Mat4 view = sbgl_CameraGetView(&cam);
    sbgl_Mat4 proj = sbgl_CameraGetProjection(&cam);

    printf("Camera Type: Orthographic\n");
    printf("Viewport: 1280x720 (top-left origin)\n");
    printf("View Matrix (first column): [%.2f, %.2f, %.2f, %.2f]\n", view.m[0][0], view.m[0][1], view.m[0][2], view.m[0][3]);
    printf("Projection Matrix (first column): [%.2f, %.2f, %.2f, %.2f]\n\n", proj.m[0][0], proj.m[0][1], proj.m[0][2], proj.m[0][3]);

    // Batch Sphere Collision
    sbgl_Ray ray;
    ray.origin = sbgl_Vec3Set(640, 360, -100); // Further back from the screen
    ray.direction = sbgl_Vec3Set(0, 0, 1);    // Looking into the screen

    sbgl_Sphere spheres[3];
    // Sphere 1: Center screen (Should hit)
    spheres[0].center = sbgl_Vec3Set(640, 360, 0);
    spheres[0].radius = 50.0f;
    // Sphere 2: Top left (Should miss)
    spheres[1].center = sbgl_Vec3Set(100, 100, 0);
    spheres[1].radius = 30.0f;
    // Sphere 3: Also center screen, but smaller (Should hit)
    spheres[2].center = sbgl_Vec3Set(640, 360, 0);
    spheres[2].radius = 10.0f;

    sbgl_HitResult results[3];
    sbgl_RaySphereIntersectBatch(ray, spheres, results, 3);

    printf("Batch Ray-Sphere Results (Simulated Mouse Click at Center):\n");
    for (int i = 0; i < 3; ++i) {
        if (results[i].hit) {
            printf("Sphere %d: HIT at distance %.2f, normal (%.1f, %.1f, %.1f)\n", 
                   i, results[i].distance, results[i].normal.x, results[i].normal.y, results[i].normal.z);
        } else {
            printf("Sphere %d: MISS\n", i);
        }
    }

    return 0;
}
