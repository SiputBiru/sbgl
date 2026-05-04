#include <stdio.h>
#include <assert.h>
#include <string.h>
#define SBL_ARENA_IMPLEMENTATION
#include "../../src/core/sbl_arena.h"

void test_arena_basic() {
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

void test_arena_growth() {
    SblArena arena;
    sbl_arena_init(&arena, 128); // Small initial size

    void* p1 = sbl_arena_alloc(&arena, 100);
    assert(p1 != NULL);

    void* p2 = sbl_arena_alloc(&arena, 100); // Should trigger growth
    assert(p2 != NULL);
    assert(arena.head->next != NULL);
    assert(arena.current != arena.head);

    void* p3 = sbl_arena_alloc(&arena, 1024); // Large allocation
    assert(p3 != NULL);

    sbl_arena_free(&arena);
}

void test_arena_reset() {
    SblArena arena;
    sbl_arena_init(&arena, 1024);

    void* p1 = sbl_arena_alloc(&arena, 512);
    void* p1_offset = (void*)arena.current->offset;

    sbl_arena_reset(&arena);
    assert(arena.current == arena.head);
    assert(arena.head->offset == 0);

    void* p2 = sbl_arena_alloc(&arena, 512);
    assert(p2 == p1); // Should reuse same memory

    sbl_arena_free(&arena);
}

int main() {
    printf("--- SBgl Arena Test ---\n");
    test_arena_basic();
    test_arena_growth();
    test_arena_reset();
    printf("All arena tests passed!\n");
    return 0;
}
