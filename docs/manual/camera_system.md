# SBgl Camera & Collision System

The camera system in SBgl provides a stateful interface for managing 3D and 2D projections. It is designed to work in conjunction with the math library to produce view and projection matrices suitable for Vulkan rendering.

## Camera Architecture

The `sbgl_Camera` structure encapsulates the necessary data to define a viewing frustum or an orthographic volume.

### Perspective Projection

Perspective cameras simulate a physical lens, where objects further away appear smaller. This is defined by a vertical field of view (FOV), an aspect ratio, and near/far clipping planes.

### Orthographic Projection

Orthographic cameras provide a parallel projection where parallel lines remain parallel and object size is independent of distance from the camera. This is typically used for 2D applications, user interfaces, or technical visualizations.

## View Management

The system utilizes a look-at model for the view matrix. By defining a position, a target point, and an up vector, the system computes the transformation required to orient the world relative to the camera's local coordinate system.

## Far Plane Management

Applications are responsible for managing the far clipping plane to ensure optimal depth buffer precision and visibility in varied environments. Architectural decoupling necessitates that the camera system remains agnostic to world size or chunked data structures.

For large-scale environments, such as voxel-based worlds, a dynamic far plane is recommended. A common heuristic for calculating the far plane distance based on the render radius and chunk size is:

`(radius + 1) * chunk_size * 1.5`

When the far plane or other projection parameters are modified within the `sbgl_Camera` structure, the projection matrix is updated during the subsequent call to `sbgl_CameraGetProjection`.

## Batch Collision Math

To adhere to Data-Oriented Design principles, the collision system is designed to process multiple objects in a single call. This minimizes function call overhead and improves cache efficiency when testing a single ray against a collection of primitive shapes.

### Intersection Testing

Supported primitives for batch testing include:

* **Spheres:** Defined by a center point and a radius.
* **AABBs:** Axis-Aligned Bounding Boxes defined by their minimum and maximum extents.

The intersection functions return a hit result containing the intersection point, the distance from the ray origin, and the surface normal at the point of impact.

## Examples

The implementation is accompanied by example applications that demonstrate the initialization and utilization of the camera and collision systems.

### Camera Example

The `camera_main.c` application illustrates the configuration of a 3D perspective camera. It demonstrates the configuration of camera parameters such as field of view and aspect ratio, and the retrieval of the resulting view and projection matrices. Additionally, a batch ray-casting test is performed against a collection of axis-aligned bounding boxes to verify 3D intersection logic and normal calculation.
