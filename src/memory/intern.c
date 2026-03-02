#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "memory/arena.h"
#include "memory/intern.h"

/** Initial hash table capacity allocated on first use. */
#define INTERN_TABLE_INIT_CAPACITY 64
/** Denominator used to calculate the load factor threshold for growing the table. */
#define INTERN_TABLE_LOAD_DENOMINATOR 2

static const uint64_t FNV_OFFSET_BASIS = 14695981039346656037ULL;
static const uint64_t FNV_PRIME        = 1099511628211ULL;

static InternPool      global_pool;
static int             global_pool_initialized = 0;
static pthread_mutex_t pool_lock               = PTHREAD_MUTEX_INITIALIZER;

/**
 * @brief Compute the FNV-1a hash of the given string data.
 *
 * Returns a non-zero 64-bit hash value.
 *
 * @param data      Pointer to the string bytes.
 * @param length    Length of the string in bytes.
 * @return uint64_t FNV-1a hash of the input data, or 1 if the computed hash is zero.
 */
static uint64_t hash(const void *data, size_t length) {
    const uint8_t *bytes    = data;
    uint64_t       hash_value = FNV_OFFSET_BASIS;

    for (size_t i = 0; i < length; i++) {
        hash_value ^= bytes[i];
        hash_value *= FNV_PRIME;
    }

    if (hash_value == 0) {
        hash_value = 1;
    }

    return hash_value;
}

/**
 * @brief Insert an atom ID into the hash table using linear probing.
 *
 * @param table    Pointer to the hash table.
 * @param hash_value FNV-1a hash of the string.
 * @param atom_id  Atom ID to insert.
 * @return int     0 on success, -1 if the table needs to be grown.
 */
static int table_insert(InternHashTable *table, uint64_t hash_value, AtomId atom_id) {
    if (table->count >= table->capacity / INTERN_TABLE_LOAD_DENOMINATOR) {
        return -1;
    }

    uint32_t mask = table->capacity - 1;
    uint32_t slot = (uint32_t)(hash_value & mask);

    while (table->hashes[slot] != 0) {
        slot = (slot + 1) & mask;
    }

    table->hashes[slot] = hash_value;
    table->atom_ids[slot]    = atom_id;
    table->count++;
    return 0;
}

/**
 * @brief Double the hash table capacity, rehash all occupied entries into the new arrays, and swap
 * the pointers.
 *
 * @return int 0 on success, -1 on allocation failure.
 */
static int table_grow(void) {
    uint32_t  new_capacity = global_pool.table.capacity << 1;
    uint64_t *new_hashes   = malloc(new_capacity * sizeof(uint64_t));
    AtomId   *new_atom_ids      = malloc(new_capacity * sizeof(AtomId));

    if (new_hashes == NULL || new_atom_ids == NULL) {
        free(new_hashes);
        free(new_atom_ids);
        return -1;
    }

    memset(new_hashes, 0, new_capacity * sizeof(uint64_t));
    memset(new_atom_ids, 0, new_capacity * sizeof(AtomId));

    uint32_t mask = new_capacity - 1;

    for (uint32_t i = 0; i < global_pool.table.capacity; i++) {
        if (global_pool.table.hashes[i] != 0) {
            uint32_t slot = (uint32_t)(global_pool.table.hashes[i] & mask);

            while (new_hashes[slot] != 0) {
                slot = (slot + 1) & mask;
            }

            new_hashes[slot] = global_pool.table.hashes[i];
            new_atom_ids[slot]    = global_pool.table.atom_ids[i];
        }
    }

    free(global_pool.table.hashes);
    free(global_pool.table.atom_ids);

    global_pool.table.hashes   = new_hashes;
    global_pool.table.atom_ids      = new_atom_ids;
    global_pool.table.capacity = new_capacity;
    return 0;
}

/**
 * @brief Grow the strings and string_lengths arrays to a new capacity, zero-initialising the new
 * slots. Returns -1 if new_capacity is not larger than the current capacity or on allocation
 * failure.
 *
 * @param new_capacity New capacity for the strings and string_lengths arrays.
 * @return int 0 on success, -1 on failure.
 */
static int strings_grow(uint32_t new_capacity) {
    if (new_capacity <= global_pool.strings_capacity) {
        return -1;
    }

    char **new_strings =
        (char **)realloc((void *)global_pool.strings, new_capacity * sizeof(char *));

    if (new_strings == NULL) {
        return -1;
    }

    global_pool.strings = new_strings;

    size_t *new_string_lengths = realloc(global_pool.string_lengths, new_capacity * sizeof(size_t));
    if (new_string_lengths == NULL) {
        return -1;
    }

    uint32_t old_capacity = global_pool.strings_capacity;

    memset((void *)(new_strings + old_capacity), 0, (new_capacity - old_capacity) * sizeof(char *));
    memset(new_string_lengths + old_capacity, 0, (new_capacity - old_capacity) * sizeof(size_t));

    global_pool.strings          = new_strings;
    global_pool.string_lengths   = new_string_lengths;
    global_pool.strings_capacity = new_capacity;
    return 0;
}

/**
 * @brief Search the hash table for a string matching a given FNV-1a hash and string content.
 *
 * Returns the existing AtomId or 0 if not found.
 *
 * @param hash_value FNV-1a hash of the string to look up.
 * @param string   Pointer to the string bytes to look up.
 * @param length   Length of the string in bytes.
 * @return AtomId  AtomId for the matching string, or 0 if not found.
 */
static AtomId intern_lookup_hash(uint64_t hash_value, const char *string, size_t length) {
    uint32_t mask = global_pool.table.capacity - 1;
    uint32_t slot = (uint32_t)(hash_value & mask);

    while (global_pool.table.hashes[slot] != 0) {
        if (global_pool.table.hashes[slot] == hash_value) {
            AtomId      atom_id = global_pool.table.atom_ids[slot];
            const char *stored  = global_pool.strings[atom_id - 1];

            if (stored != NULL && memcmp(stored, string, length) == 0 && stored[length] == '\0') {
                return atom_id;
            }
        }
        slot = (slot + 1) & mask;
    }

    return 0;
}

/**
 * @brief Assign a new AtomId for a string and insert it into the hash table and strings array.
 *
 * Returns the new AtomId, or 0 on failure. Assumes the caller has already checked that the string
 * is not already interned.
 *
 * @param string   Pointer to the string bytes to intern.
 * @param length   Length of the string in bytes.
 * @param hash_value FNV-1a hash of the string.
 * @return AtomId  A new AtomId for the string, or 0 on failure.
 */
static AtomId intern_assign_id(const char *string, size_t length, uint64_t hash_value) {
    char *destination = arena_alloc(&global_pool.arena, length + 1);

    if (destination == NULL) {
        return 0;
    }

    memcpy(destination, string, length);
    destination[length] = '\0';

    /* IDs are 1-based. 0 is reserved as the invalid/null id. */
    AtomId atom_id = (AtomId)(global_pool.table.count + 1);

    if (atom_id > global_pool.strings_capacity) {
        uint32_t new_capacity = global_pool.strings_capacity == 0
                                    ? INTERN_TABLE_INIT_CAPACITY
                                    : global_pool.strings_capacity << 1;
        if (strings_grow(new_capacity) != 0) {
            return 0;
        }
        if (atom_id - 1 >= global_pool.strings_capacity) {
            return 0;
        }
    }

    if (table_insert(&global_pool.table, hash_value, atom_id) != 0) {
        return 0;
    }

    global_pool.strings[atom_id - 1]        = destination;
    global_pool.string_lengths[atom_id - 1] = length;
    return atom_id;
}

const char *intern_lookup(AtomId atom_id, size_t *length) {
    pthread_mutex_lock(&pool_lock);
    if (!global_pool_initialized || atom_id == 0 || atom_id > global_pool.strings_capacity) {
        pthread_mutex_unlock(&pool_lock);
        return NULL;
    }

    const char *string = global_pool.strings[atom_id - 1];
    if (string == NULL) {
        pthread_mutex_unlock(&pool_lock);
        return NULL;
    }

    if (length != NULL) {
        *length = global_pool.string_lengths[atom_id - 1];
    }

    pthread_mutex_unlock(&pool_lock);
    return string;
}

int intern_init(void) {
    pthread_mutex_lock(&pool_lock);
    if (global_pool_initialized) {
        pthread_mutex_unlock(&pool_lock);
        return 0;
    }

    global_pool.arena = arena_init(ARENA_DEFAULT_CAPACITY);

    if (global_pool.arena.base == NULL) {
        pthread_mutex_unlock(&pool_lock);
        return -1;
    }

    uint32_t initial_capacity = INTERN_TABLE_INIT_CAPACITY;
    global_pool.table.hashes  = malloc(initial_capacity * sizeof(uint64_t));
    global_pool.table.atom_ids     = malloc(initial_capacity * sizeof(AtomId));

    if (global_pool.table.hashes == NULL || global_pool.table.atom_ids == NULL) {
        free(global_pool.table.hashes);
        free(global_pool.table.atom_ids);
        pthread_mutex_unlock(&pool_lock);
        return -1;
    }

    memset(global_pool.table.hashes, 0, initial_capacity * sizeof(uint64_t));
    memset(global_pool.table.atom_ids, 0, initial_capacity * sizeof(AtomId));

    global_pool.table.count      = 0;
    global_pool.table.capacity   = initial_capacity;
    global_pool.strings          = NULL;
    global_pool.string_lengths   = NULL;
    global_pool.strings_capacity = 0;
    global_pool.reference_count  = 0;
    global_pool_initialized      = 1;
    pthread_mutex_unlock(&pool_lock);
    return 0;
}

AtomId intern_string(const char *string, size_t length) {
    pthread_mutex_lock(&pool_lock);
    if (!global_pool_initialized) {
        pthread_mutex_unlock(&pool_lock);
        return 0;
    }

    uint64_t hash_value = hash(string, length);

    AtomId atom_id = intern_lookup_hash(hash_value, string, length);
    if (atom_id != 0) {
        pthread_mutex_unlock(&pool_lock);
        return atom_id;
    }

    if (global_pool.table.count >= global_pool.table.capacity / INTERN_TABLE_LOAD_DENOMINATOR) {
        if (table_grow() != 0) {
            pthread_mutex_unlock(&pool_lock);
            return 0;
        }
    }

    atom_id = intern_assign_id(string, length, hash_value);
    pthread_mutex_unlock(&pool_lock);
    return atom_id;
}

void intern_retain(void) {
    pthread_mutex_lock(&pool_lock);
    if (global_pool_initialized) {
        global_pool.reference_count++;
    }
    pthread_mutex_unlock(&pool_lock);
}

void intern_release(void) {
    pthread_mutex_lock(&pool_lock);
    if (!global_pool_initialized) {
        pthread_mutex_unlock(&pool_lock);
        return;
    }

    if (global_pool.reference_count > 0) {
        global_pool.reference_count--;
    }

    if (global_pool.reference_count == 0) {
        free(global_pool.table.hashes);
        free(global_pool.table.atom_ids);
        free((void *)global_pool.strings);
        free(global_pool.string_lengths);
        arena_free(&global_pool.arena);
        global_pool             = (InternPool){0};
        global_pool_initialized = 0;
    }
    pthread_mutex_unlock(&pool_lock);
}
