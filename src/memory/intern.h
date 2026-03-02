#pragma once

#include <stddef.h>
#include <stdint.h>

#include "memory/arena.h"

/**
 * @brief AtomId is a 32-bit identifier for an interned string.
 *
 * Zero is reserved as an invalid/null id.
 */
typedef uint32_t AtomId;

/**
 * @brief Open-addressed hash table for string interning.
 *
 * Uses linear probing. Capacity is always a power of two to allow mask-based indexing. A hash
 * value of zero indicates an empty slot. Hash equality is a necessary but not sufficient
 * condition for a match, a full byte comparison always confirms identity. Two distinct strings with
 * the same hash (a true collision) are exceedingly rare but handled correctly by the probe.
 */
typedef struct InternHashTable {
    uint64_t *hashes;   /**< Full 64-bit hashes (0 = empty slot). */
    AtomId   *atom_ids;      /**< AtomId for each occupied slot. */
    uint32_t  count;    /**< Number of occupied slots. */
    uint32_t  capacity; /**< Total slot count, always a power of two. */
} InternHashTable;

/**
 * @brief Global string interning pool.
 *
 * Maps string content to stable AtomIds using an open-addressed hash table. Duplicate strings
 * across all parses share the same AtomId.
 *
 * Memory layout:
 * - String content is arena-allocated (bump, never freed individually).
 * - Hash table arrays (hashes, atom_ids) and string index arrays (strings, string_lengths) are
 *   malloc/realloc managed so old copies are freed on each doubling, avoiding arena bloat and
 *   mmap-induced latency spikes.
 *
 * Ownership is reference counted. The pool frees itself when the last reference is released. All
 * public intern_* functions are thread-safe.
 */
typedef struct InternPool {
    Arena           arena;          /**< Bump allocator for string content only. */
    InternHashTable table;          /**< Hash table for content-to-id lookup hashes and atom_ids. */
    char          **strings;        /**< strings[id-1] points to the interned string for that id. */
    size_t         *string_lengths; /**< string_lengths[id-1] is the byte length of that string. */
    uint32_t        strings_capacity; /**< Allocated capacity of the strings array. */
    uint32_t        reference_count;  /**< Number of active references. */
} InternPool;

/**
 * @brief Initialises the global intern pool.
 *
 * Called automatically by sexp_parse. May be called explicitly, subsequent calls while the pool
 * is active are no-ops.
 *
 * @return Zero on success, non-zero on failure.
 */
int intern_init(void);


/**
 * @brief Interns a string, returning a stable AtomId.
 *
 * @param string Pointer to the string bytes.
 * @param length Length of the string in bytes.
 * @return       A stable AtomId, or 0 on failure.
 */
AtomId intern_string(const char *string, size_t length);


/**
 * @brief Looks up the string content for a given AtomId.
 *
 * The returned pointer is valid for the lifetime of the pool.
 *
 * @param atom_id ID of the atom whose string to retrieve.
 * @param length  Output parameter for string length in bytes.
 * @return        Pointer to the interned string bytes, or NULL if invalid.
 */
const char *intern_lookup(AtomId atom_id, size_t *length);


/**
 * @brief Retains a reference to the global intern pool.
 *
 * Increments the reference count. Each retain must be paired with a corresponding release.
 */
void intern_retain(void);


/**
 * @brief Releases a reference to the global intern pool.
 *
 * Decrements the reference count. When it reaches zero, the pool frees all owned memory.
 */
void intern_release(void);
