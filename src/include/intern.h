#pragma once
#include <stddef.h>
#include <stdint.h>
#include "arena.h"

/**
 * Identifier for an interned string.
 *
 * Zero is reserved as an invalid/null id.
 */
typedef uint32_t AtomId;

/**
 * Open-addressed hash table for string interning.
 *
 * Uses linear probing. Capacity is always a power of two to allow mask-based 
 * indexing.
 *
 * A hash value of zero indicates an empty slot. Two strings with the same hash
 * are a true collision and are handled by probing - the hash alone does not 
 * guarantee uniqueness, though the probability of a collision is negligible.
 */
typedef struct InternHashTable {
    uint64_t *hashes; /**< Full 64-bit hashes (0 = empty slot). */
    AtomId   *ids;    /**< AtomId for each occupied slot. */
    uint32_t  count;  /**< Number of occupied slots. */
    uint32_t  cap;    /**< Total slot count, always a power of two. */
} InternHashTable;

/**
 * Global string interning pool.
 *
 * Maps string content to stable AtomIds using an open-addressed hash table 
 * backed by an Arena. Duplicate strings across all parses share the same 
 * AtomId.
 *
 * Uses generational eviction to bound memory growth. Atoms unseen for max_gen 
 * parses are candidates for compaction. A bitmask tracks liveness per 
 * generation in O(n/64) time.
 *
 * Ownership is reference counted. The pool frees itself when the last 
 * reference is released.
 */
typedef struct InternPool {
    Arena           arena;      /**< Backing memory for strings and table. */
    InternHashTable table;      /**< Hash table for content-to-id lookup. */
    uint32_t       *live_mask;  /**< Liveness bitmask, one bit per AtomId. */
    uint32_t        ref_count;  /**< Number of active references. */
    uint32_t        generation; /**< Current parse generation counter. */
    uint32_t        max_gen;    /**< Generations before eligible to evict. */
} InternPool;
