#include <stdio.h>
#include <assert.h>
#include "sbgl_batcher.h"

/**
 * Verifies the baking logic by providing a set of sorted draw packets.
 * 
 * The test defines packets with varying mesh, material, and sort keys to 
 * ensure that grouping occurs only when all batching criteria are met.
 */
void test_bake_commands() {
    sbgl_DrawPacket packets[5];
    
    // Group 1: Two identical packets
    packets[0] = (sbgl_DrawPacket){ .key = 10, .meshId = 1, .materialId = 1, .instanceId = 100 };
    packets[1] = (sbgl_DrawPacket){ .key = 10, .meshId = 1, .materialId = 1, .instanceId = 101 };
    
    // Group 2: Different mesh
    packets[2] = (sbgl_DrawPacket){ .key = 10, .meshId = 2, .materialId = 1, .instanceId = 102 };
    
    // Group 3: Different material
    packets[3] = (sbgl_DrawPacket){ .key = 10, .meshId = 2, .materialId = 2, .instanceId = 103 };
    
    // Group 4: Different key
    packets[4] = (sbgl_DrawPacket){ .key = 11, .meshId = 2, .materialId = 2, .instanceId = 104 };

    sbgl_IndirectCommand commands[5];
    uint32_t count = sbgl_bake_commands(packets, 5, commands, 5);

    // The system should identify 4 distinct command groups based on state changes.
    assert(count == 4);

    // Group 1 verification (2 instances, mesh 1)
    assert(commands[0].instanceCount == 2);
    assert(commands[0].indexCount == 1 * 10);
    assert(commands[0].firstInstance == 0);

    // Group 2 verification (1 instance, mesh 2)
    assert(commands[1].instanceCount == 1);
    assert(commands[1].indexCount == 2 * 10);
    assert(commands[1].firstInstance == 2);

    // Group 3 verification (1 instance, mesh 2, new material)
    assert(commands[2].instanceCount == 1);
    assert(commands[2].indexCount == 2 * 10);
    assert(commands[2].firstInstance == 3);

    // Group 4 verification (1 instance, mesh 2, new sort key)
    assert(commands[3].instanceCount == 1);
    assert(commands[3].indexCount == 2 * 10);
    assert(commands[3].firstInstance == 4);

    printf("PASS: test_bake_commands\n");
}

int main() {
    test_bake_commands();
    return 0;
}
