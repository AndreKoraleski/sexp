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
 * Contiguous storage for interned strings.
 *
 * Strings are stored back-to-back and null-terminated, memory is allocated 
 * from an external Arena and individual deallocation is not supported.
 */
typedef struct AtomStorage {
    char   *base;  /**< Start of the string buffer. */
    size_t  cap;   /**< Total capacity in bytes. */
    size_t  pos;   /**< Next write offset in bytes. */
} AtomStorage;


/**
 * Open-addressed hash table for string interning.
 *
 * Uses linear probing. Capacity is always a power of two to allow mask-based 
 * indexing.
 *
 * If a hash slot is zero, it is considered empty. This allows us to avoid 
 * storing the string content in the hash table, as the full 64-bit hash is 
 * sufficient for collision resolution.
 */
typedef struct InternHashTable {
    uint64_t *hashes; /**< Full 64-bit hashes (0 = empty slot). */
    AtomId   *ids;    /**< AtomId for each occupied slot. */
    uint32_t  count;  /**< Number of occupied slots. */
    uint32_t  cap;    /**< Total slot count (power of two). */
} InternHashTable;


/**
 * String interning pool.
 *
 * Maps string content to stable AtomIds. Duplicate strings share the same id.
 *
 * Combines contiguous storage for string data and an open-addressed hash table
 * for deduplication.
 *
 * All memory is allocated from a provided Arena.
 */
typedef struct InternPool {
    AtomStorage     storage; /**< Contiguous backing store for string bytes. */
    InternHashTable table;   /**< Hash table for deduplication lookups. */
} InternPool;
