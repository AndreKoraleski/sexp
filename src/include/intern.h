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
 * Ownership is reference counted. The pool frees itself when the last 
 * reference is released.
 */
typedef struct InternPool {
    Arena           arena;       /**< Backing memory for strings and table. */
    InternHashTable table;       /**< Hash table for content-to-id lookup. */
    char          **strings;     /**< strings[id-1] points to the
                                  *   interned string for that id. */
    size_t         *string_lens; /**< string_lens[id-1] is the byte
                                  *   length of that string. */
    uint32_t        strings_cap; /**< Allocated capacity of the
                                  *   strings array. */
    uint32_t        ref_count;   /**< Number of active references. */
} InternPool;

/**
 * Initialises the global intern pool.
 *
 * Must be called once before any parsing.
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
 * The returned pointer is valid for the lifetime of the pool.
 *
 * @param id    AtomId to look up.
 * @param len   Output parameter for string length in bytes.
 * @return      Pointer to the interned string bytes, or NULL if invalid.
 */
const char *intern_lookup(AtomId id, size_t *len);

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
