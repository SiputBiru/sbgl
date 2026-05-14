/**
 * @file sbgl_camera.h
 * @brief Camera system and batch collision math for SBgl.
 *
 * Provides a stateful camera system supporting both perspective and
 * orthographic projections, alongside data-oriented collision testing
 * for rays, spheres, and axis-aligned bounding boxes.
 */

#ifndef SBGL_CAMERA_H
#define SBGL_CAMERA_H

#include "sbgl_math.h"

/**
 * @brief Camera projection types.
 */
typedef enum { SBGL_CAMERA_PERSPECTIVE, SBGL_CAMERA_ORTHOGRAPHIC } sbgl_CameraType;

/**
 * @brief Stateful camera representation.
 *
 * Stores the necessary parameters to construct view and projection matrices.
 * The orientation is typically managed via position, target, and up vectors
 * to generate a look-at matrix.
 */
typedef struct {
	sbgl_CameraType type;

	sbgl_Vec3 position;
	sbgl_Vec3 target;
	sbgl_Vec3 up;

	float fov_y;
	float aspect;
	float near_plane;
	float far_plane;

	float ortho_left;
	float ortho_right;
	float ortho_bottom;
	float ortho_top;
} sbgl_Camera;

/**
 * @brief Mathematical ray.
 */
typedef struct {
	sbgl_Vec3 origin;
	sbgl_Vec3 direction;
} sbgl_Ray;

/**
 * @brief Axis-Aligned Bounding Box (AABB).
 */
typedef struct {
	sbgl_Vec3 min;
	sbgl_Vec3 max;
} sbgl_AABB;

/**
 * @brief Bounding sphere.
 */
typedef struct {
	sbgl_Vec3 center;
	float radius;
} sbgl_Sphere;

/**
 * @brief Intersection result for batch testing.
 */
typedef struct {
	bool hit;
	float distance;
	sbgl_Vec3 point;
	sbgl_Vec3 normal;
} sbgl_HitResult;

/**
 * @brief Initializes a camera with perspective projection.
 *
 * Sets the camera to use a perspective frustum defined by the field of view,
 * aspect ratio, and clipping planes.
 */
static inline sbgl_Camera
sbgl_CameraPerspective(float fov_y_rad, float aspect, float near_p, float far_p) {
	sbgl_Camera cam = { 0 };
	cam.type = SBGL_CAMERA_PERSPECTIVE;
	cam.position = sbgl_Vec3Set(0, 0, 5);
	cam.target = sbgl_Vec3Set(0, 0, 0);
	cam.up = sbgl_Vec3Set(0, 1, 0);
	cam.fov_y = fov_y_rad;
	cam.aspect = aspect;
	cam.near_plane = near_p;
	cam.far_plane = far_p;
	return cam;
}

/**
 * @brief Initializes a camera with orthographic projection.
 *
 * Sets the camera to use a parallel projection defined by the viewport
 * boundaries and clipping planes.
 */
static inline sbgl_Camera sbgl_CameraOrthographic(sbgl_OrthoParams p) {
	sbgl_Camera cam = { 0 };
	cam.type = SBGL_CAMERA_ORTHOGRAPHIC;
	cam.position = sbgl_Vec3Set(0, 0, 1);
	cam.target = sbgl_Vec3Set(0, 0, 0);
	cam.up = sbgl_Vec3Set(0, 1, 0);
	cam.ortho_left = p.left;
	cam.ortho_right = p.right;
	cam.ortho_bottom = p.bottom;
	cam.ortho_top = p.top;
	cam.near_plane = p.near_p;
	cam.far_plane = p.far_p;
	return cam;
}

/**
 * @brief Computes the view matrix for the given camera.
 *
 * Generates a look-at matrix based on the current position, target, and up
 * vectors of the camera state.
 */
static inline sbgl_Mat4 sbgl_CameraGetView(const sbgl_Camera* cam) {
	return sbgl_Mat4LookAt(cam->position, cam->target, cam->up);
}

/**
 * @brief Computes the projection matrix for the given camera.
 *
 * Generates either a perspective or orthographic matrix based on the
 * internal camera type and its associated parameters.
 */
static inline sbgl_Mat4 sbgl_CameraGetProjection(const sbgl_Camera* cam) {
	if (cam->type == SBGL_CAMERA_PERSPECTIVE) {
		return sbgl_Mat4Perspective(cam->fov_y, cam->aspect, cam->near_plane, cam->far_plane);
	}
	sbgl_OrthoParams p = { .left = cam->ortho_left,
						   .right = cam->ortho_right,
						   .bottom = cam->ortho_bottom,
						   .top = cam->ortho_top,
						   .near_p = cam->near_plane,
						   .far_p = cam->far_plane };
	return sbgl_Mat4Orthographic(p);
}

/**
 * @brief Performs a batch ray-sphere intersection test.
 *
 * Tests a single ray against an array of spheres, populating the results
 * array with hit information for each sphere.
 */
static inline void sbgl_RaySphereIntersectBatch(
	sbgl_Ray ray,
	const sbgl_Sphere* spheres,
	sbgl_HitResult* results,
	uint32_t count
) {
	for (uint32_t i = 0; i < count; ++i) {
		sbgl_Vec3 oc = sbgl_Vec3Sub(ray.origin, spheres[i].center);
		float a = sbgl_Vec3Dot(ray.direction, ray.direction);
		float b = 2.0f * sbgl_Vec3Dot(oc, ray.direction);
		float c = sbgl_Vec3Dot(oc, oc) - spheres[i].radius * spheres[i].radius;
		float discriminant = b * b - 4 * a * c;

		results[i].hit = false;
		if (discriminant >= 0) {
			float sqrt_d = sqrtf(discriminant);
			float t0 = (-b - sqrt_d) / (2.0f * a);
			float t1 = (-b + sqrt_d) / (2.0f * a);

			float t = t0;
			if (t < 0)
				t = t1;

			if (t > 0) {
				results[i].hit = true;
				results[i].distance = t;
				results[i].point = sbgl_Vec3Add(ray.origin, sbgl_Vec3Mul(ray.direction, t));
				results[i].normal =
					sbgl_Vec3Normalize(sbgl_Vec3Sub(results[i].point, spheres[i].center));
			}
		}
	}
}

/**
 * @brief Performs a batch ray-AABB intersection test.
 *
 * Tests a single ray against an array of axis-aligned bounding boxes using
 * the slab method. Populates the results array with intersection data.
 */
static inline void sbgl_RayAABBIntersectBatch(
	sbgl_Ray ray,
	const sbgl_AABB* boxes,
	sbgl_HitResult* results,
	uint32_t count
) {
	sbgl_Vec3 inv_dir = {
		{ 1.0f / ray.direction.x, 1.0f / ray.direction.y, 1.0f / ray.direction.z, 0.0f }
	};

	for (uint32_t i = 0; i < count; ++i) {
		float t1 = (boxes[i].min.x - ray.origin.x) * inv_dir.x;
		float t2 = (boxes[i].max.x - ray.origin.x) * inv_dir.x;
		float tmin = fminf(t1, t2);
		float tmax = fmaxf(t1, t2);

		t1 = (boxes[i].min.y - ray.origin.y) * inv_dir.y;
		t2 = (boxes[i].max.y - ray.origin.y) * inv_dir.y;
		tmin = fmaxf(tmin, fminf(t1, t2));
		tmax = fminf(tmax, fmaxf(t1, t2));

		t1 = (boxes[i].min.z - ray.origin.z) * inv_dir.z;
		t2 = (boxes[i].max.z - ray.origin.z) * inv_dir.z;
		tmin = fmaxf(tmin, fminf(t1, t2));
		tmax = fminf(tmax, fmaxf(t1, t2));

		results[i].hit = (tmax >= tmin && tmax > 0);
		if (results[i].hit) {
			results[i].distance = tmin > 0 ? tmin : tmax;
			results[i].point =
				sbgl_Vec3Add(ray.origin, sbgl_Vec3Mul(ray.direction, results[i].distance));

			// Calculate normal based on which face was hit
			sbgl_Vec3 center = sbgl_Vec3Mul(sbgl_Vec3Add(boxes[i].min, boxes[i].max), 0.5f);
			sbgl_Vec3 p = sbgl_Vec3Sub(results[i].point, center);
			sbgl_Vec3 d = sbgl_Vec3Mul(sbgl_Vec3Sub(boxes[i].max, boxes[i].min), 0.5f);
			float epsilon = 0.0001f;

			results[i].normal = sbgl_Vec3Set(0, 0, 0);
			if (fabsf(p.x) >= fabsf(d.x) - epsilon)
				results[i].normal.x = p.x > 0 ? 1.0f : -1.0f;
			else if (fabsf(p.y) >= fabsf(d.y) - epsilon)
				results[i].normal.y = p.y > 0 ? 1.0f : -1.0f;
			else if (fabsf(p.z) >= fabsf(d.z) - epsilon)
				results[i].normal.z = p.z > 0 ? 1.0f : -1.0f;
		}
	}
}

#endif // SBGL_CAMERA_H
