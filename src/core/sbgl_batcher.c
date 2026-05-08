#include "sbgl_batcher.h"

uint32_t sbgl_bake_commands(
    const sbgl_DrawPacket* packets,
    uint32_t packetCount,
    sbgl_IndirectCommand* outCommands,
    uint32_t maxCommands
) {
    if (packetCount == 0 || maxCommands == 0) {
        return 0;
    }

    // The system initializes the first command using the first packet in the stream.
    // Geometry parameters are derived from the mesh identifier.
    uint32_t commandCount = 1;
    
    switch (packets[0].meshId) {
        case 0: // Triangle
            outCommands[0].indexCount = 3;
            outCommands[0].firstIndex = 0;
            outCommands[0].vertexOffset = 0;
            break;
        case 1: // Cube
            outCommands[0].indexCount = 36;
            outCommands[0].firstIndex = 3;
            outCommands[0].vertexOffset = 3;
            break;
        case 2: // Pyramid
            outCommands[0].indexCount = 18;
            outCommands[0].firstIndex = 39;
            outCommands[0].vertexOffset = 11;
            break;
        case 3: // Voxel Chunk (Procedural 32x32 grid of cubes)
            outCommands[0].indexCount = 36864; // 32 * 32 * 36
            outCommands[0].firstIndex = 0;    // Generated in shader via gl_VertexIndex
            outCommands[0].vertexOffset = 0;
            break;
        default:

            outCommands[0].indexCount = 0;
            outCommands[0].firstIndex = 0;
            outCommands[0].vertexOffset = 0;
            break;
    }
    
    outCommands[0].instanceCount = 1;
    outCommands[0].firstInstance = 0;

    // The logic iterates through remaining packets, comparing each to its predecessor.
    // If the state matches, the instance count of the current command is incremented.
    // Otherwise, a new command is started if space permits in the output buffer.
    for (uint32_t i = 1; i < packetCount; ++i) {
        const sbgl_DrawPacket* packet = &packets[i];
        const sbgl_DrawPacket* previous = &packets[i - 1];

        bool canBatch = (packet->meshId == previous->meshId) &&
                        (packet->materialId == previous->materialId) &&
                        (packet->key == previous->key);

        if (canBatch) {
            outCommands[commandCount - 1].instanceCount++;
        } else {
            if (commandCount >= maxCommands) {
                break;
            }

            sbgl_IndirectCommand* command = &outCommands[commandCount];
            
            switch (packet->meshId) {
                case 0: // Triangle
                    command->indexCount = 3;
                    command->firstIndex = 0;
                    command->vertexOffset = 0;
                    break;
                case 1: // Cube
                    command->indexCount = 36;
                    command->firstIndex = 3;
                    command->vertexOffset = 3;
                    break;
                case 2: // Pyramid
                    command->indexCount = 18;
                    command->firstIndex = 39;
                    command->vertexOffset = 11;
                    break;
                case 3: // Voxel Chunk
                    command->indexCount = 36864;
                    command->firstIndex = 0;
                    command->vertexOffset = 0;
                    break;
                default:
                    command->indexCount = 0;
                    command->firstIndex = 0;
                    command->vertexOffset = 0;
                    break;
            }
            
            command->instanceCount = 1;
            
            // The starting instance index is set to the current packet index,
            // assuming instance data is packed contiguously in the order of sorted packets.
            command->firstInstance = i;

            commandCount++;
        }
    }

    return commandCount;
}
