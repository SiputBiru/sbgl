#ifndef SBGL_BATCHER_H
#define SBGL_BATCHER_H

/**
 * @file sbgl_batcher.h
 * @brief The baking pipeline for compressing draw packets into indirect commands.
 *
 * The batcher provides the logic for grouping multiple draw requests into 
 * consolidated indirect commands, maximizing GPU throughput.
 */

#include "sbgl_types.h"
#include "backend/sbgl_graphics_hal.h"

/**
 * Bakes an array of sorted draw packets into optimized indirect commands.
 * 
 * The system groups contiguous draw packets that share the same mesh, material, 
 * and sort key into single multi-instance draw calls. This reduction minimizes
 * the overhead of submitting thousands of individual commands to the graphics 
 * hardware.
 * 
 * @param packets A contiguous array of draw packets, assumed to be sorted.
 * @param packetCount Total number of packets provided in the input array.
 * @param outCommands Destination array for generated indirect commands.
 * @param maxCommands Maximum number of commands the output array can hold.
 * @return The total number of indirect commands generated.
 */
uint32_t sbgl_bake_commands(
    const sbgl_DrawPacket* packets,
    uint32_t packetCount,
    sbgl_IndirectCommand* outCommands,
    uint32_t maxCommands
);

#endif // SBGL_BATCHER_H
