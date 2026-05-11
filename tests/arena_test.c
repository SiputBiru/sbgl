#include <stdio.h>
#include <assert.h>
#include <string.h>
#define SBL_ARENA_IMPLEMENTATION
#include "../src/core/sbl_arena.h"

static void test_arena_basic(void) {
    SblArena arena;
    sbl_arena_init(&arena, 1024);
    assert(arena.head != NULL);
    assert(arena.current == arena.head);

    void* p1 = sbl_arena_alloc(&arena, 256);
    assert(p1 != NULL);
    memset(p1, 0xAA, 256);

    void* p2 = sbl_arena_alloc(&arena, 512);
    assert(p2 != NULL);
    assert(p2 != p1);
    memset(p2, 0xBB, 512);

    sbl_arena_free(&arena);
}

static void test_arena_growth(void) {
    SblArena arena;
    sbl_arena_init(&arena, 128); // Small initial size

    void* p1 = sbl_arena_alloc(&arena, 100);
    if (!p1) assert(0);
    memset(p1, 0x1, 100);

    void* p2 = sbl_arena_alloc(&arena, 100); // Should trigger growth
    if (!p2) assert(0);
    memset(p2, 0x2, 100);
    assert(arena.head->next != NULL);
    assert(arena.current != arena.head);

    void* p3 = sbl_arena_alloc(&arena, 1024); // Large allocation
    if (!p3) assert(0);
    memset(p3, 0x3, 1024);

    sbl_arena_free(&arena);
}

static void test_arena_reset(void) {
    SblArena arena;
    sbl_arena_init(&arena, 1024);

    void* p1 = sbl_arena_alloc(&arena, 512);
    if (!p1) assert(0);
    memset(p1, 0xAA, 512);

    sbl_arena_reset(&arena);
    assert(arena.current == arena.head);
    assert(arena.head->offset == 0);

    void* p2 = sbl_arena_alloc(&arena, 512);
    if (p2 != p1) {
        printf("Arena reset failed to reuse memory: p1=%p, p2=%p\n", p1, p2);
        assert(0);
    }
    memset(p2, 0xBB, 512);

    sbl_arena_free(&arena);
}

static void test_arena_multi_block_reuse(void) {
    SblArena arena;
    sbl_arena_init(&arena, 128);

    // Grow to 3 blocks
    void* p1 = sbl_arena_alloc(&arena, 100);
    void* p2 = sbl_arena_alloc(&arena, 100); // 2nd block (256 bytes)
    void* p3 = sbl_arena_alloc(&arena, 300); // 3rd block (512 bytes)
    (void)p2; (void)p3;
    assert(arena.head->next != NULL);
    assert(arena.head->next->next != NULL);

    SblArenaBlock* block2 = arena.head->next;
    SblArenaBlock* block3 = block2->next;

    sbl_arena_reset(&arena);

    // Reuse 1st block
    void* r1 = sbl_arena_alloc(&arena, 100);
    assert(r1 == p1);

    // Reuse 2nd block
    void* r2 = sbl_arena_alloc(&arena, 100);
    (void)r2;
    assert(arena.current == block2);

    // Reuse 3rd block
    void* r3 = sbl_arena_alloc(&arena, 300);
    (void)r3;
    assert(arena.current == block3);

    sbl_arena_reset(&arena);

    // Now test "free old chain" if size increases beyond previous growth
    sbl_arena_alloc(&arena, 100); // 1st block
    sbl_arena_alloc(&arena, 100); // reuse 2nd block (256 bytes)
    
    // This allocation (600 bytes) is larger than the 3rd block (512 bytes)
    // It should free block3 and create a new one.
    void* r4 = sbl_arena_alloc(&arena, 600); 
    (void)r4;
    assert(arena.current != block3);
    assert(block2->next != block3);
    assert(arena.current->size >= 1200); // 600 * 2

    sbl_arena_free(&arena);
}

int main(void) {
    printf("--- SBgl Arena Test ---\n");
    test_arena_basic();
    test_arena_growth();
    test_arena_reset();
    test_arena_multi_block_reuse();
    printf("All arena tests passed!\n");
    return 0;
}
