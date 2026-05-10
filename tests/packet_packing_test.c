#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include "sbgl_types.h"

int main(void) {
    uint32_t mesh = 1024;
    uint32_t mat = 512;
    uint32_t blend = 2;
    uint32_t sided = 1;
    uint32_t tags = 42;

    // These macros should be defined in sbgl_types.h
    uint32_t header = SBGL_PACK_HEADER(mesh, mat, blend, sided, tags);

    assert(SBGL_GET_MESH_ID(header) == mesh);
    assert(SBGL_GET_MAT_ID(header) == mat);
    assert(SBGL_GET_BLEND_MODE(header) == blend);
    
    // Verify structure size
    sbgl_DrawPacket packet;
    if (sizeof(packet) != 16) {
        printf("DrawPacket size mismatch: expected 16, got %zu\n", sizeof(packet));
        assert(0);
    }
    
    // Verify vertex size
    assert(sizeof(sbgl_Vertex) == 16);

    // Verify packing/unpacking via packet
    packet.header = header;
    if (SBGL_GET_MESH_ID(packet.header) != mesh) {
        printf("Mesh ID mismatch in packet\n");
        assert(0);
    }
    assert(SBGL_GET_MAT_ID(packet.header) == mat);
    assert(SBGL_GET_BLEND_MODE(packet.header) == blend);

    printf("Bit-packed header test passed!\n");
    return 0;
}
