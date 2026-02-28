#pragma once
#include <stddef.h>
#include <stdint.h>

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
