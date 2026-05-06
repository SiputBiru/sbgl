# Triangle Rendering Guide

This modular guide details the rendering of a triangle.

## Shader Loading

```c
size_t v_sz, f_sz;
uint32_t* v_spv = read_file("shader.vert.spv", &v_sz);
sbgl_Shader v_shd = sbgl_LoadShader(ctx, SBGL_SHADER_STAGE_VERTEX, v_spv, v_sz);
```

## Pipeline Creation

```c
sbgl_PipelineConfig cfg = { .vertexShader = v_shd, .fragmentShader = f_shd };
sbgl_Pipeline pip = sbgl_CreatePipeline(ctx, &cfg);
```

## Drawing

```c
sbgl_BeginDrawing(ctx);
sbgl_BindPipeline(ctx, pip);
sbgl_Draw(ctx, 3, 0);
sbgl_EndDrawing(ctx);
```
