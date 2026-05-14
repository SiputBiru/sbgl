/**
 * @file sbgl_context_internal.h
 * @brief Internal helpers for accessing context state from library subsystems.
 *
 * These declarations are for internal use only and must not be included
 * by external code.
 */

#ifndef SBGL_CONTEXT_INTERNAL_H
#define SBGL_CONTEXT_INTERNAL_H

#include "sbgl_types.h"
#include "core/sbl_arena.h"

/**
 * @brief Retrieves the persistent arena associated with a context.
 *
 * Subsystems that need a lifetime-bound allocation region (e.g., the voxel
 * engine) use this arena instead of malloc to remain consistent with the
 * library's memory model.
 *
 * @param ctx The engine context.
 * @return Pointer to the context's persistent arena, or NULL if the context is invalid.
 */
SblArena* sbgl_GetContextArena(sbgl_Context* ctx);

#endif // SBGL_CONTEXT_INTERNAL_H
