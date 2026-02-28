#pragma once
#include <stddef.h>
#include <stdint.h>
#include "arena.h"

#define INTERN_DEFAULT_MAX_GEN 1 /* Retain atoms for 1 generation. */

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

/**
 * Initialises the global intern pool.
 *
 * Must be called once before any parsing. Uses INTERN_DEFAULT_MAX_GEN as the 
 * initial generation limit.
 *
 * @return Zero on success, non-zero on failure.
 */
int intern_init(void);

/**
 * Interns a string, returning a stable AtomId.
 *
 * If the string is already interned, returns the existing AtomId.
 * If not, copies the string into the pool and assigns a new AtomId.
 *
 * @param str  Pointer to the string bytes.
 * @param len  Length of the string in bytes.
 * @return     A stable AtomId, or 0 on failure.
 */
AtomId intern_string(const char *str, size_t len);

/**
 * Looks up the string content for a given AtomId.
 *
 * The returned pointer is valid until the next compaction.
 *
 * @param id    AtomId to look up.
 * @param len   Output parameter for string length in bytes.
 * @return      Pointer to the interned string bytes, or NULL if invalid.
 */
const char *intern_lookup(AtomId id, size_t *len);

/**
 * Checks whether two AtomIds refer to equal strings.
 *
 * Equality is a simple integer comparison.
 *
 * @param a  First AtomId.
 * @param b  Second AtomId.
 * @return   Non-zero if equal, zero if not.
 */
int intern_equal(AtomId a, AtomId b);

/**
 * Advances the generation counter of the global pool.
 *
 * Should be called once after each completed parse to mark which atoms were 
 * seen in the current generation.
 */
void intern_advance_generation(void);

/**
 * Compacts the pool, evicting atoms unseen for max_gen generations.
 *
 * Rebuilds the hash table and string storage keeping only live atoms. All 
 * AtomIds for evicted atoms are invalidated after this call.
 */
void intern_compact(void);

/**
 * Sets the maximum number of generations before an atom is eviction eligible.
 *
 * Default is INTERN_DEFAULT_MAX_GEN. Higher values trade memory for more cache
 * hits across parses with stable vocabularies.
 *
 * @param n  Number of generations to retain.
 */
void intern_set_max_generations(uint32_t n);

/**
 * Retains a reference to the global pool.
 *
 * Increments the reference count. Each retain must be paired with a 
 * corresponding release.
 */
void intern_retain(void);

/**
 * Releases a reference to the global pool.
 *
 * Decrements the reference count. When it reaches zero the pool frees all 
 * owned memory.
 */
void intern_release(void);
