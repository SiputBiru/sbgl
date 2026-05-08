# Triangle Rendering Guide

This modular guide details the rendering of a triangle.

## Shader Loading

SBgl provides a helper to load shaders directly from SPIR-V files.

```c
sbgl_Shader v_shd = sbgl_LoadShaderFromFile(ctx, SBGL_SHADER_STAGE_VERTEX, "shader.vert.spv");
sbgl_Shader f_shd = sbgl_LoadShaderFromFile(ctx, SBGL_SHADER_STAGE_FRAGMENT, "shader.frag.spv");
```

## Vertex Layout

A vertex layout describes how the GPU should interpret raw vertex data. The system utilizes the `sbgl_Vertex` structure, which is optimized for cache density (16 bytes). It consists of a 4D position (3D + padding) using 16-bit signed normalized integers (SNORM) and a packed RGBA8 color.

```c
sbgl_VertexAttribute attributes[] = {
    { .location = 0, .format = SBGL_FORMAT_R16G16B16A16_SNORM, .offset = offsetof(sbgl_Vertex, position) },
    { .location = 1, .format = SBGL_FORMAT_R8G8B8A8_UNORM, .offset = offsetof(sbgl_Vertex, color) }
};

sbgl_VertexLayout layout = {
    .stride = sizeof(sbgl_Vertex),
    .attributeCount = 2,
    .attributes = attributes
};
```

## Pipeline Creation

The graphics pipeline encapsulates the shader stages and the vertex layout.

```c
sbgl_PipelineConfig cfg = { 
    .vertexShader = v_shd, 
    .fragmentShader = f_shd,
    .vertexLayout = layout
};
sbgl_Pipeline pip = sbgl_CreatePipeline(ctx, &cfg);
```

## Vertex Buffer

Vertices are initialized using the `sbgl_Vertex` structure. Positions are quantized to the 16-bit range (`-32767` to `32767`), representing `-1.0` to `1.0`.

```c
sbgl_Vertex vertices[] = {
    { .position = { 0, 16383, 0, 0 },    .color = 0xFF0000FF }, // Top (Red)
    { .position = { 16383, -16383, 0, 0 }, .color = 0xFF00FF00 }, // Bottom Right (Green)
    { .position = { -16383, -16383, 0, 0 }, .color = 0xFFFF0000 }  // Bottom Left (Blue)
};

sbgl_Buffer vbo = sbgl_CreateBuffer(ctx, SBGL_BUFFER_USAGE_VERTEX, sizeof(vertices), vertices);
```

## Drawing

```c
sbgl_BeginDrawing(ctx);
sbgl_BindPipeline(ctx, pip);
sbgl_BindBuffer(ctx, vbo, SBGL_BUFFER_USAGE_VERTEX);
sbgl_Draw(ctx, 3, 0);
sbgl_EndDrawing(ctx);
```

## Resource Cleanup

Resources must be destroyed in the reverse order of their creation after the GPU has finished executing all commands.

```c
// Ensure the GPU is no longer using any resources
sbgl_DeviceWaitIdle(ctx);

sbgl_DestroyBuffer(ctx, vbo);
sbgl_DestroyPipeline(ctx, pip);
sbgl_DestroyShader(ctx, v_shd);
sbgl_DestroyShader(ctx, f_shd);

// Shutdown the engine and close the window
sbgl_Shutdown(ctx);
```
