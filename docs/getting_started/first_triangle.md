# Triangle Rendering Guide

This modular guide details the rendering of a triangle.

## Shader Loading

SBgl loads shaders from SPIR-V bytecode. The `read_file` helper from `examples/example_util.h` is used here for brevity.

```c
size_t v_sz, f_sz;
uint32_t* v_spv = read_file("shader.vert.spv", &v_sz);
uint32_t* f_spv = read_file("shader.frag.spv", &f_sz);

sbgl_Shader v_shd = sbgl_LoadShader(ctx, SBGL_SHADER_STAGE_VERTEX, v_spv, v_sz);
sbgl_Shader f_shd = sbgl_LoadShader(ctx, SBGL_SHADER_STAGE_FRAGMENT, f_spv, f_sz);
```

## Vertex Layout

A vertex layout describes how the GPU should interpret raw vertex data. In this example, each vertex consists of a 2D position (2 floats) followed by an RGB color (3 floats).

```c
sbgl_VertexAttribute attributes[] = {
    { .location = 0, .format = SBGL_FORMAT_R32G32_SFLOAT, .offset = 0 },
    { .location = 1, .format = SBGL_FORMAT_R32G32B32_SFLOAT, .offset = 2 * sizeof(float) }
};

sbgl_VertexLayout layout = {
    .stride = 5 * sizeof(float),
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

```c
float vertices[] = {
     0.0f, -0.5f, 1.0f, 0.0f, 0.0f,
     0.5f,  0.5f, 0.0f, 1.0f, 0.0f,
    -0.5f,  0.5f, 0.0f, 0.0f, 1.0f
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
