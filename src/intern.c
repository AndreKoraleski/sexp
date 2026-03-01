#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "arena.h"
#include "intern.h"

#define INTERN_TABLE_INIT_CAP  64
#define INTERN_TABLE_LOAD_DENOM 2

static const uint64_t FNV_OFFSET_BASIS = 14695981039346656037ULL;
static const uint64_t FNV_PRIME        = 1099511628211ULL;

static InternPool global_pool;
static int global_pool_initialized = 0;
static pthread_mutex_t pool_lock = PTHREAD_MUTEX_INITIALIZER;

static uint64_t hash(const void *data, size_t len) {
    const uint8_t *bytes = data;
    uint64_t fnv_hash = FNV_OFFSET_BASIS;

    for (size_t i = 0; i < len; i++) {
        fnv_hash ^= bytes[i];
        fnv_hash *= FNV_PRIME;
    }

    if (fnv_hash == 0) {
        fnv_hash = 1;
    }

    return fnv_hash;
}

static int table_insert(
    InternHashTable *table,
    uint64_t fnv_hash,
    AtomId atom_id
) {
    if (table->count >= table->cap / INTERN_TABLE_LOAD_DENOM) {
        return -1;
    }

    uint32_t mask = table->cap - 1;
    uint32_t slot = (uint32_t)(fnv_hash & mask);
    while (table->hashes[slot] != 0) {
        slot = (slot + 1) & mask;
    }
    table->hashes[slot] = fnv_hash;
    table->ids[slot]    = atom_id;
    table->count++;
    return 0;
}

static int table_grow(void) {
    uint32_t  new_cap    = global_pool.table.cap << 1;
    uint64_t *new_hashes = malloc(new_cap * sizeof(uint64_t));
    AtomId   *new_ids    = malloc(new_cap * sizeof(AtomId));

    if (new_hashes == NULL || new_ids == NULL) {
        free(new_hashes);
        free(new_ids);
        return -1;
    }

    memset(new_hashes, 0, new_cap * sizeof(uint64_t));
    memset(new_ids,    0, new_cap * sizeof(AtomId));

    uint32_t mask = new_cap - 1;
    for (uint32_t i = 0; i < global_pool.table.cap; i++) {
        if (global_pool.table.hashes[i] != 0) {
            uint32_t slot =
                (uint32_t)(global_pool.table.hashes[i] & mask);

                while (new_hashes[slot] != 0) {
                slot = (slot + 1) & mask;
            }
            new_hashes[slot] = global_pool.table.hashes[i];
            new_ids[slot]    = global_pool.table.ids[i];
        }
    }

    free(global_pool.table.hashes);
    free(global_pool.table.ids);
    global_pool.table.hashes = new_hashes;
    global_pool.table.ids    = new_ids;
    global_pool.table.cap    = new_cap;
    return 0;
}

static int strings_grow(uint32_t new_cap) {
    if (new_cap <= global_pool.strings_cap) {
        return -1;
    }

    char **new_strings = realloc(
        global_pool.strings,
        new_cap * sizeof(char *)
    );
    if (new_strings == NULL) {
        return -1;
    }
    global_pool.strings = new_strings;

    size_t *new_lens = realloc(
        global_pool.string_lens,
        new_cap * sizeof(size_t)
    );
    if (new_lens == NULL) {
        return -1;
    }

    uint32_t old_cap = global_pool.strings_cap;
    memset(new_strings + old_cap, 0, (new_cap - old_cap) * sizeof(char *));
    memset(new_lens    + old_cap, 0, (new_cap - old_cap) * sizeof(size_t));

    global_pool.strings     = new_strings;
    global_pool.string_lens = new_lens;
    global_pool.strings_cap = new_cap;
    return 0;
}

static AtomId intern_lookup_hash(
    uint64_t fnv_hash,
    const char *str,
    size_t len
) {
    uint32_t mask = global_pool.table.cap - 1;
    uint32_t slot = (uint32_t)(fnv_hash & mask);

    while (global_pool.table.hashes[slot] != 0) {
        if (global_pool.table.hashes[slot] == fnv_hash) {
            AtomId atom_id  = global_pool.table.ids[slot];
            const char *stored = global_pool.strings[atom_id - 1];
            if (stored != NULL
                    && memcmp(stored, str, len) == 0
                    && stored[len] == '\0') {
                return atom_id;
            }
        }
        slot = (slot + 1) & mask;
    }

    return 0;
}

static AtomId intern_assign_id(
    const char *str,
    size_t len,
    uint64_t fnv_hash
) {
    char *dest = arena_alloc(&global_pool.arena, len + 1);
    if (dest == NULL) {
        return 0;
    }

    memcpy(dest, str, len);
    dest[len] = '\0';

    AtomId atom_id = (AtomId)(global_pool.table.count + 1);

    if (atom_id > global_pool.strings_cap) {
        uint32_t new_cap = global_pool.strings_cap == 0
            ? INTERN_TABLE_INIT_CAP
            : global_pool.strings_cap << 1;
        if (strings_grow(new_cap) != 0) {
            return 0;
        }
        if (atom_id - 1 >= global_pool.strings_cap) {
            return 0;
        }
    }

    if (table_insert(&global_pool.table, fnv_hash, atom_id) != 0) {
        return 0;
    }

    global_pool.strings[atom_id - 1]     = dest;
    global_pool.string_lens[atom_id - 1] = len;
    return atom_id;
}

const char *intern_lookup(AtomId atom_id, size_t *len) {
    pthread_mutex_lock(&pool_lock);
    if (!global_pool_initialized
            || atom_id == 0
            || atom_id > global_pool.strings_cap) {
        pthread_mutex_unlock(&pool_lock);
        return NULL;
    }

    const char *str = global_pool.strings[atom_id - 1];
    if (str == NULL) {
        pthread_mutex_unlock(&pool_lock);
        return NULL;
    }

    if (len != NULL) {
        *len = global_pool.string_lens[atom_id - 1];
    }

    pthread_mutex_unlock(&pool_lock);
    return str;
}

int intern_init(void) {
    pthread_mutex_lock(&pool_lock);
    if (global_pool_initialized) {
        pthread_mutex_unlock(&pool_lock);
        return 0;
    }

    global_pool.arena = arena_init(ARENA_DEFAULT_CAP);

    if (global_pool.arena.base == NULL) {
        pthread_mutex_unlock(&pool_lock);
        return -1;
    }

    uint32_t initial_cap = INTERN_TABLE_INIT_CAP;
    global_pool.table.hashes = malloc(initial_cap * sizeof(uint64_t));
    global_pool.table.ids    = malloc(initial_cap * sizeof(AtomId));

    if (global_pool.table.hashes == NULL
            || global_pool.table.ids == NULL) {
        free(global_pool.table.hashes);
        free(global_pool.table.ids);
        pthread_mutex_unlock(&pool_lock);
        return -1;
    }

    memset(global_pool.table.hashes, 0,
        initial_cap * sizeof(uint64_t));
    memset(global_pool.table.ids, 0,
        initial_cap * sizeof(AtomId));

    global_pool.table.count = 0;
    global_pool.table.cap   = initial_cap;
    global_pool.strings     = NULL;
    global_pool.string_lens = NULL;
    global_pool.strings_cap = 0;
    global_pool.ref_count   = 0;
    global_pool_initialized = 1;
    pthread_mutex_unlock(&pool_lock);
    return 0;
}

AtomId intern_string(const char *str, size_t len) {
    pthread_mutex_lock(&pool_lock);
    if (!global_pool_initialized) {
        pthread_mutex_unlock(&pool_lock);
        return 0;
    }

    uint64_t fnv_hash = hash(str, len);

    AtomId atom_id = intern_lookup_hash(fnv_hash, str, len);
    if (atom_id != 0) {
        pthread_mutex_unlock(&pool_lock);
        return atom_id;
    }

    if (global_pool.table.count >=
            global_pool.table.cap / INTERN_TABLE_LOAD_DENOM) {
        if (table_grow() != 0) {
            pthread_mutex_unlock(&pool_lock);
            return 0;
        }
    }

    atom_id = intern_assign_id(str, len, fnv_hash);
    pthread_mutex_unlock(&pool_lock);
    return atom_id;
}

void intern_retain(void) {
    pthread_mutex_lock(&pool_lock);
    if (global_pool_initialized) {
        global_pool.ref_count++;
    }
    pthread_mutex_unlock(&pool_lock);
}

void intern_release(void) {
    pthread_mutex_lock(&pool_lock);
    if (!global_pool_initialized) {
        pthread_mutex_unlock(&pool_lock);
        return;
    }

    if (global_pool.ref_count > 0) {
        global_pool.ref_count--;
    }

    if (global_pool.ref_count == 0) {
        free(global_pool.table.hashes);
        free(global_pool.table.ids);
        free(global_pool.strings);
        free(global_pool.string_lens);
        arena_free(&global_pool.arena);
        global_pool = (InternPool){0};
        global_pool_initialized = 0;
    }
    pthread_mutex_unlock(&pool_lock);
}
