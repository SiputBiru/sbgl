# Performance Telemetry System

SBgl provides a high-precision telemetry system designed to identify CPU and GPU bottlenecks in real-time. By utilizing hardware-native timing mechanisms, the system delivers ground-truth performance data with minimal execution overhead.

---

## Architecture

The telemetry system independently measures three primary components of the frame lifecycle:

1.  **GPU Execution Time**: Measures the absolute duration the hardware was processing commands. This is implemented using **Vulkan Timestamp Queries** (`vkCmdWriteTimestamp`). To prevent CPU stalls, results are read back asynchronously from the previous frame.
2.  **CPU Frame Duration**: Measures the total time between `sbgl_BeginDrawing` and `sbgl_EndDrawing`.
3.  **CPU Sorting/Baking Overhead**: Measures the specific duration spent in the Data-Oriented sorting and indirect command generation phase.

### Timing Precision
- **Linux**: Utilizes `clock_gettime` with `CLOCK_MONOTONIC`.
- **GPU**: Precision is determined by the hardware's `timestampPeriod` (nanoseconds per tick).

---

## The Telemetry Structure

Performance metrics are encapsulated in the `sbgl_Telemetry` structure:

```c
typedef struct {
    float cpu_frame_time;    /**< Total frame duration (ms). */
    float cpu_sort_time;     /**< Time spent in sort and bake (ms). */
    float gpu_render_time;   /**< Actual GPU execution time (ms). */
    uint32_t draw_calls;     /**< Total MDI batches submitted. */
    uint32_t instance_count; /**< Total instances rendered. */
} sbgl_Telemetry;
```

---

## Usage Example

Telemetry data for the *previous* frame is retrieved via the engine context. This asynchronous pattern ensures that the CPU never waits for the GPU to finish its work before starting the next frame's logic.

```c
#include "sbgl.h"
#include <stdio.h>

int main() {
    sbgl_Context* ctx = sbgl_Init(...).ctx;

    while (!sbgl_WindowShouldClose(ctx)) {
        sbgl_BeginDrawing(ctx);
        
        // ... rendering logic ...

        sbgl_EndDrawing(ctx);

        // Retrieve and display metrics
        sbgl_Telemetry stats = sbgl_GetTelemetry(ctx);
        printf("CPU: %.2fms | GPU: %.2fms | Batches: %u\n", 
               stats.cpu_frame_time, 
               stats.gpu_render_time,
               stats.draw_calls);
    }
    
    sbgl_Shutdown(ctx);
    return 0;
}
```

---

## Bottleneck Identification

The telemetry data allows for clear identification of the limiting factor:

| Observation | Probable Bottleneck | Recommended Action |
| :--- | :--- | :--- |
| High `gpu_render_time` | Fragment Shading / Fill-rate | Reduce render resolution or simplify shaders. |
| High `cpu_sort_time` | Batcher Overhead | Optimize sorting keys or reduce total draw packets. |
| Large `cpu_frame_time` difference | Driver / CPU Logic | Profile application-level update logic or driver submission. |

---

## Future Considerations

Planned extensions include **Granular Profiling**, which will enable timestamp markers for individual render queues and compute passes, allowing for deep-dive analysis of complex frame structures.
