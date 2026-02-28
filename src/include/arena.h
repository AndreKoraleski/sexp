#pragma once

#include <stddef.h>
#include <stdint.h>

/**
 * Linear (bump) allocator backed by a fixed-size buffer.
 *
 * This allocator provides O(1) allocation by advancing a cursor through a 
 * contiguous memory region.
 *
 * Allocations are sequential, individual frees are not supported, and all 
 * memory is reclaimed at once via reset.
 *
 * The caller owns the backing buffer.
 */
typedef struct LinearArena {
    uint8_t *base;  /**< Start of the backing buffer. */
    size_t   cap;   /**< Total buffer capacity in bytes. */
    size_t   pos;   /**< Current allocation offset in bytes. */
} LinearArena;
