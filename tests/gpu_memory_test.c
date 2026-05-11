#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

/* Mocked definitions for standalone testing of the managed heap logic. */
#define SBGL_INVALID_OFFSET ((uint32_t)-1)
#define SBGL_ERROR_OUT_OF_MEMORY 4
#define SBGL_MANAGED_HEAP_SIZE (256 * 1024 * 1024)

typedef struct {
  uint32_t offset;
  uint32_t size;
  uint32_t handle_index; // 0 if free
  uint32_t _padding;
} sbgl_GfxMemoryRange;

typedef struct {
  uint32_t size;
  sbgl_GfxMemoryRange ranges[1024];
  uint32_t rangeCount;
} sbgl_GfxManagedHeap;

typedef struct {
  sbgl_GfxManagedHeap managedHeap;
  int result;
} sbgl_GfxContext;

/* The following functions are duplicated from the Vulkan backend to facilitate 
   standalone logic verification without requiring a full Vulkan environment. */

static uint32_t managed_heap_alloc(sbgl_GfxContext* ctx, size_t size) {
  uint32_t alignedSize = (uint32_t)((size + 255) & ~255);

  for (uint32_t i = 0; i < ctx->managedHeap.rangeCount; i++) {
    sbgl_GfxMemoryRange* range = &ctx->managedHeap.ranges[i];

    if (range->handle_index == 0 && range->size >= alignedSize) {
      uint32_t offset = range->offset;

      if (range->size > alignedSize) {
        if (ctx->managedHeap.rangeCount >= 1024) {
          ctx->result = SBGL_ERROR_OUT_OF_MEMORY;
          return SBGL_INVALID_OFFSET;
        }

        for (uint32_t j = ctx->managedHeap.rangeCount; j > i + 1; j--) {
          ctx->managedHeap.ranges[j] = ctx->managedHeap.ranges[j - 1];
        }

        ctx->managedHeap.ranges[i + 1] = (sbgl_GfxMemoryRange){
          .offset = offset + alignedSize,
          .size = range->size - alignedSize,
          .handle_index = 0
        };

        ctx->managedHeap.rangeCount++;
        range->size = alignedSize;
      }

      range->handle_index = 1;
      return offset;
    }
  }

  ctx->result = SBGL_ERROR_OUT_OF_MEMORY;
  return SBGL_INVALID_OFFSET;
}

static void managed_heap_free(sbgl_GfxContext* ctx, uint32_t offset) {
  for (uint32_t i = 0; i < ctx->managedHeap.rangeCount; i++) {
    if (ctx->managedHeap.ranges[i].offset == offset) {
      ctx->managedHeap.ranges[i].handle_index = 0;

      /* Merge with next neighbor if free. */
      if (i + 1 < ctx->managedHeap.rangeCount && ctx->managedHeap.ranges[i + 1].handle_index == 0) {
        ctx->managedHeap.ranges[i].size += ctx->managedHeap.ranges[i + 1].size;
        for (uint32_t j = i + 1; j < ctx->managedHeap.rangeCount - 1; j++) {
          ctx->managedHeap.ranges[j] = ctx->managedHeap.ranges[j + 1];
        }
        ctx->managedHeap.rangeCount--;
      }

      /* Merge with previous neighbor if free. */
      if (i > 0 && ctx->managedHeap.ranges[i - 1].handle_index == 0) {
        ctx->managedHeap.ranges[i - 1].size += ctx->managedHeap.ranges[i].size;
        for (uint32_t j = i; j < ctx->managedHeap.rangeCount - 1; j++) {
          ctx->managedHeap.ranges[j] = ctx->managedHeap.ranges[j + 1];
        }
        ctx->managedHeap.rangeCount--;
      }

      return;
    }
  }
}

static void test_managed_heap_basic(void) {
  sbgl_GfxContext ctx = {0};
  ctx.managedHeap.size = SBGL_MANAGED_HEAP_SIZE;
  ctx.managedHeap.rangeCount = 1;
  ctx.managedHeap.ranges[0] = (sbgl_GfxMemoryRange){ .offset = 0, .size = SBGL_MANAGED_HEAP_SIZE, .handle_index = 0 };

  /* Test basic allocation. */
  uint32_t off1 = managed_heap_alloc(&ctx, 1024);
  assert(off1 == 0);
  assert(ctx.managedHeap.ranges[0].size == 1024);
  assert(ctx.managedHeap.ranges[0].handle_index == 1);
  assert(ctx.managedHeap.rangeCount == 2);
  assert(ctx.managedHeap.ranges[1].offset == 1024);
  assert(ctx.managedHeap.ranges[1].size == SBGL_MANAGED_HEAP_SIZE - 1024);

  /* Test second allocation. */
  uint32_t off2 = managed_heap_alloc(&ctx, 2048);
  assert(off2 == 1024);
  assert(ctx.managedHeap.ranges[1].size == 2048);
  assert(ctx.managedHeap.rangeCount == 3);

  /* Test freeing and merging (next). */
  managed_heap_free(&ctx, off2);
  assert(ctx.managedHeap.rangeCount == 2);
  assert(ctx.managedHeap.ranges[1].offset == 1024);
  assert(ctx.managedHeap.ranges[1].size == SBGL_MANAGED_HEAP_SIZE - 1024);
  assert(ctx.managedHeap.ranges[1].handle_index == 0);

  /* Test freeing and merging (previous). */
  managed_heap_free(&ctx, off1);
  assert(ctx.managedHeap.rangeCount == 1);
  assert(ctx.managedHeap.ranges[0].offset == 0);
  assert(ctx.managedHeap.ranges[0].size == SBGL_MANAGED_HEAP_SIZE);
  assert(ctx.managedHeap.ranges[0].handle_index == 0);

  printf("Managed Heap Basic Test: PASSED\n");
}

static void test_managed_heap_fragmentation(void) {
  sbgl_GfxContext ctx = {0};
  ctx.managedHeap.size = SBGL_MANAGED_HEAP_SIZE;
  ctx.managedHeap.rangeCount = 1;
  ctx.managedHeap.ranges[0] = (sbgl_GfxMemoryRange){ .offset = 0, .size = SBGL_MANAGED_HEAP_SIZE, .handle_index = 0 };

  uint32_t a = managed_heap_alloc(&ctx, 1024);
  uint32_t b = managed_heap_alloc(&ctx, 1024);
  uint32_t c = managed_heap_alloc(&ctx, 1024);

  /* Free middle block. */
  managed_heap_free(&ctx, b);
  assert(ctx.managedHeap.rangeCount == 4);
  assert(ctx.managedHeap.ranges[1].handle_index == 0);

  /* Free first block (should merge with middle). */
  managed_heap_free(&ctx, a);
  assert(ctx.managedHeap.rangeCount == 3);
  assert(ctx.managedHeap.ranges[0].size == 2048);
  assert(ctx.managedHeap.ranges[0].handle_index == 0);

  /* Free last block (should merge all into one). */
  managed_heap_free(&ctx, c);
  assert(ctx.managedHeap.rangeCount == 1);
  assert(ctx.managedHeap.ranges[0].size == SBGL_MANAGED_HEAP_SIZE);

  printf("Managed Heap Fragmentation Test: PASSED\n");
}

int main(void) {
  test_managed_heap_basic();
  test_managed_heap_fragmentation();
  return 0;
}
