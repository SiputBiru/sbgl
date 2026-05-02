#ifndef SBL_ARENA_H
#define SBL_ARENA_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#ifndef SBL_ARENA_DEF
#define SBL_ARENA_DEF
#endif

// --- Macros ---

#define SBL_ARENA_PUSH_STRUCT(arena, type) ((type*)sbl_arena_alloc((arena), sizeof(type)))
#define SBL_ARENA_PUSH_STRUCT_ZERO(arena, type) ((type*)sbl_arena_alloc_zero((arena), sizeof(type)))
#define SBL_ARENA_PUSH_ARRAY(arena, type, count)                                                   \
	((type*)sbl_arena_alloc((arena), sizeof(type) * (count)))
#define SBL_ARENA_PUSH_ARRAY_ZERO(arena, type, count)                                              \
	((type*)sbl_arena_alloc_zero((arena), sizeof(type) * (count)))

#define SBL_ARENA_PUSH_STRING(arena, str)                                                          \
	((char*)SBL_ARENA_MEMCPY(sbl_arena_alloc((arena), strlen(str) + 1), (str), strlen(str) + 1))

// --- Types ---

/**
 * @brief A single block of memory in the arena.
 */
typedef struct SblArenaBlock {
	uint64_t size;               /**< Total capacity of this block. */
	uint64_t offset;             /**< Current allocation offset. */
	struct SblArenaBlock* next;  /**< Pointer to the next block in the chain. */
} SblArenaBlock;

/**
 * @brief High-speed arena allocator.
 * 
 * Manages a chain of memory blocks to provide fast, linear allocations
 * without individual free calls.
 */
typedef struct SblArena {
	SblArenaBlock* head;         /**< The first block in the arena. */
	SblArenaBlock* current;      /**< The current active block. */
} SblArena;

/**
 * @brief Bookmark for arena state.
 * 
 * Used to "rewind" the arena to a specific point.
 */
typedef struct {
	SblArenaBlock* block;        /**< The block at the time of marking. */
	uint64_t offset;             /**< The offset at the time of marking. */
} SblArenaMark;

// Thread-local arena (extern in header unless static)
#ifndef SBL_ARENA_STATIC
#if __STDC_VERSION__ >= 201112L
extern _Thread_local SblArena sbl_thread_arena;
extern _Thread_local int sbl_thread_arena_initialized;
#else
extern __thread SblArena sbl_thread_arena;
extern __thread int sbl_thread_arena_initialized;
#endif
#endif

// --- Core API ---

SBL_ARENA_DEF void sbl_arena_init(SblArena* arena, uint64_t initial_size);
SBL_ARENA_DEF void sbl_arena_free(SblArena* arena);
SBL_ARENA_DEF void* sbl_arena_alloc(SblArena* arena, uint64_t size);
SBL_ARENA_DEF void* sbl_arena_alloc_zero(SblArena* arena, uint64_t size);
SBL_ARENA_DEF void sbl_arena_reset(SblArena* arena);

SBL_ARENA_DEF SblArenaMark sbl_arena_mark(SblArena* arena);
SBL_ARENA_DEF void sbl_arena_rewind(SblArena* arena, SblArenaMark mark);

SBL_ARENA_DEF SblArena* sbl_get_thread_arena(void);

// Allow to override memset/memcpy
#ifndef SBL_ARENA_MEMSET
#include <string.h>
#define SBL_ARENA_MEMSET(dst, val, sz) memset(dst, val, sz)
#endif

#ifndef SBL_ARENA_MEMCPY
#include <string.h>
#define SBL_ARENA_MEMCPY(dst, src, sz) memcpy(dst, src, sz)
#endif

#endif // SBL_ARENA_H

#ifdef SBL_ARENA_IMPLEMENTATION

#include <stdlib.h>

static uintptr_t sbl_arena__align_forward(uintptr_t ptr, uint64_t align) {
	uintptr_t p = ptr;
	uintptr_t a = (uintptr_t)align;
	uintptr_t mod = p & (a - 1);
	if (mod > 0) {
		p += (a - mod);
	}
	return p;
}

static SblArenaBlock* sbl_arena__block_create(uint64_t size) {
	uint64_t total_size = size + sizeof(SblArenaBlock);
	SblArenaBlock* block = (SblArenaBlock*)malloc(total_size);
	if (!block) return NULL;
	block->size = size;
	block->offset = 0;
	block->next = NULL;
	return block;
}

void sbl_arena_init(SblArena* arena, uint64_t initial_size) {
	arena->head = sbl_arena__block_create(initial_size);
	arena->current = arena->head;
}

void sbl_arena_free(SblArena* arena) {
	if (!arena) return;
	SblArenaBlock* b = arena->head;
	while (b) {
		SblArenaBlock* next = b->next;
		free(b);
		b = next;
	}
}

void* sbl_arena_alloc(SblArena* arena, uint64_t size) {
	uint64_t align = 8;
	SblArenaBlock* b = arena->current;

	uintptr_t curr_ptr = (uintptr_t)b + sizeof(SblArenaBlock) + b->offset;
	uintptr_t aligned_ptr = sbl_arena__align_forward(curr_ptr, align);
	uint64_t aligned_offset = aligned_ptr - ((uintptr_t)b + sizeof(SblArenaBlock));

	if (aligned_offset + size > b->size) {
		uint64_t next_size = b->size * 2;
		if (size > next_size) next_size = size * 2;
		SblArenaBlock* next = sbl_arena__block_create(next_size);
		b->next = next;
		arena->current = next;
		b = next;
		aligned_ptr = (uintptr_t)b + sizeof(SblArenaBlock);
	}

	void* ptr = (void*)aligned_ptr;
	b->offset = (aligned_ptr + size) - ((uintptr_t)b + sizeof(SblArenaBlock));
	return ptr;
}

void* sbl_arena_alloc_zero(SblArena* arena, uint64_t size) {
	void* ptr = sbl_arena_alloc(arena, size);
	if (ptr) SBL_ARENA_MEMSET(ptr, 0, size);
	return ptr;
}

void sbl_arena_reset(SblArena* arena) {
	SblArenaBlock* b = arena->head;
	while (b) {
		b->offset = 0;
		b = b->next;
	}
	arena->current = arena->head;
}

SblArenaMark sbl_arena_mark(SblArena* arena) {
	SblArenaMark mark;
	mark.block = arena->current;
	mark.offset = arena->current->offset;
	return mark;
}

void sbl_arena_rewind(SblArena* arena, SblArenaMark mark) {
	arena->current = mark.block;
	arena->current->offset = mark.offset;
}

#ifndef SBL_ARENA_STATIC
#if __STDC_VERSION__ >= 201112L
_Thread_local SblArena sbl_thread_arena;
_Thread_local int sbl_thread_arena_initialized = 0;
#else
__thread SblArena sbl_thread_arena;
__thread int sbl_thread_arena_initialized = 0;
#endif

SblArena* sbl_get_thread_arena(void) {
	if (!sbl_thread_arena_initialized) {
		sbl_arena_init(&sbl_thread_arena, 1024 * 1024);
		sbl_thread_arena_initialized = 1;
	}
	return &sbl_thread_arena;
}
#endif

#endif // SBL_ARENA_IMPLEMENTATION
