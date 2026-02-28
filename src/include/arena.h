#pragma once
#include <stddef.h>
#include <stdint.h>

#define ARENA_DEFAULT_CAP 65536 /* 64KB */

/**
 * Linear (bump) allocator with automatic chunk-based growth.
 *
 * Provides O(1) allocation by advancing a cursor through a contiguous memory 
 * chunk. When a chunk is exhausted, a new chunk of double the capacity is 
 * allocated and linked.
 *
 * Individual frees are not supported. All memory is reclaimed at once via 
 * reset or free.
 *
 * Memory is owned and managed internally.
 */
typedef struct Arena {
    uint8_t      *base; /**< Start of the current chunk. */
    size_t        cap;  /**< Capacity of the current chunk in bytes. */
    size_t        pos;  /**< Current allocation offset in bytes. */
    struct Arena *prev; /**< Previous chunk, or NULL if first. */
} Arena;

/**
 * Initialises an arena with the given initial capacity.
 *
 * Pass ARENA_DEFAULT_CAP if no specific size estimate is available. The arena 
 * will grow automatically as needed. Returns a zeroed struct on failure.
 * 
 * @param cap  Initial capacity in bytes.
 * @return     An initialised Arena, or a zeroed struct on failure.
 */
Arena arena_init(size_t cap);

/**
 * Allocates a block of memory from the arena.
 *
 * If the current chunk is exhausted, a new chunk of double the capacity is 
 * allocated and linked. Returns NULL only if system memory is exhausted.
 *  *
 * @param arena  Pointer to the arena to allocate from.
 * @param size   Number of bytes to allocate.
 * @return       Aligned pointer to allocated memory, or NULL on failure.
 */
void *arena_alloc(Arena *arena, size_t size);

/**
 * Resets the arena, reclaiming all memory at once.
 *
 * Frees all chunks except the first, which is retained and reset for immediate
 * reuse. All pointers previously returned by arena_alloc are invalidated.
 *
 * @param arena  Pointer to the arena to reset.
 */
void arena_reset(Arena *arena);

/**
 * Frees all memory owned by the arena.
 *
 * Releases all chunks including the first. The arena must not be used after 
 * this call.
 *
 * @param arena  Pointer to the arena to free.
 */
void arena_free(Arena *arena);
