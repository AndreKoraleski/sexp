#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "arena.h"
#include "intern.h"

#define INTERN_TABLE_INIT_CAP  64
#define INTERN_TABLE_MAX_LOAD   2

static const uint64_t FNV_OFFSET_BASIS = 14695981039346656037ULL;
static const uint64_t FNV_PRIME        = 1099511628211ULL;

static InternPool global_pool;
static int global_pool_initialized = 0;

static uint64_t hash(const void *data, size_t len) {
    const uint8_t *bytes = data;
    uint64_t h = FNV_OFFSET_BASIS;

    for (size_t i = 0; i < len; i++) {
        h ^= bytes[i];
        h *= FNV_PRIME;
    }

    if (h == 0)
        h = 1;

    return h;
}

static int table_insert(InternHashTable *table, uint64_t h, AtomId id) {
    if (table->count >= table->cap / INTERN_TABLE_MAX_LOAD)
        return -1;

    uint32_t mask = table->cap - 1;
    uint32_t slot = (uint32_t)(h & mask);
    while (table->hashes[slot] != 0)
        slot = (slot + 1) & mask;
    table->hashes[slot] = h;

    table->ids[slot]    = id;
    table->count++;
    return 0;
}

static int table_grow(void) {
    uint32_t new_cap = global_pool.table.cap << 1;
    uint64_t *new_hashes = arena_alloc(
        &global_pool.arena,
        new_cap * sizeof(uint64_t)
    );
    AtomId *new_ids = arena_alloc(
        &global_pool.arena,
        new_cap * sizeof(AtomId)
    );

    if (new_hashes == NULL || new_ids == NULL)
        return -1;

    for (uint32_t i = 0; i < new_cap; i++) {
        new_hashes[i] = 0;
        new_ids[i]    = 0;
    }

    uint32_t mask = new_cap - 1;
    for (uint32_t i = 0; i < global_pool.table.cap; i++) {
        if (global_pool.table.hashes[i] != 0) {
            uint32_t slot = (uint32_t)(global_pool.table.hashes[i] & mask);
            while (new_hashes[slot] != 0)
                slot = (slot + 1) & mask;
            new_hashes[slot] = global_pool.table.hashes[i];
            new_ids[slot]    = global_pool.table.ids[i];
        }
    }

    global_pool.table.hashes = new_hashes;
    global_pool.table.ids    = new_ids;
    global_pool.table.cap    = new_cap;
    return 0;
}

static int strings_grow(uint32_t needed_cap) {
    char **new_strings = arena_alloc(
        &global_pool.arena,
        needed_cap * sizeof(char *)
    );
    if (new_strings == NULL)
        return -1;

    uint32_t old_cap = global_pool.strings_cap;
    for (uint32_t i = 0; i < old_cap; i++)
        new_strings[i] = global_pool.strings[i];
    for (uint32_t i = old_cap; i < needed_cap; i++)
        new_strings[i] = NULL;

    global_pool.strings     = new_strings;
    global_pool.strings_cap = needed_cap;
    return 0;
}

static AtomId intern_lookup_hash(uint64_t h, const char *str, size_t len) {
    uint32_t mask = global_pool.table.cap - 1;
    uint32_t slot = (uint32_t)(h & mask);

    while (global_pool.table.hashes[slot] != 0) {
        if (global_pool.table.hashes[slot] == h) {
            AtomId id = global_pool.table.ids[slot];
            const char *stored = global_pool.strings[id - 1];
            if (
                stored != NULL
                && memcmp(stored, str, len) == 0
                    
                && stored[len] == '\0'
            )
                return id;
        }
        slot = (slot + 1) & mask;
    }

    return 0;
}

static AtomId intern_assign_id(const char *str, size_t len, uint64_t h) {
    char *dest = arena_alloc(&global_pool.arena, len + 1);
    if (dest == NULL)
        return 0;

    for (size_t i = 0; i < len; i++)
        dest[i] = str[i];
    dest[len] = '\0';

    AtomId id = (AtomId)(global_pool.table.count + 1);

    if (id > global_pool.strings_cap)
        if (strings_grow(id) != 0)
            return 0;

    if (table_insert(&global_pool.table, h, id) != 0)
        return 0;

    global_pool.strings[id - 1] = dest;
    return id;
}

const char *intern_lookup(AtomId id, size_t *len) {
    if (!global_pool_initialized || id == 0 || id > global_pool.strings_cap)
        return NULL;

    const char *str = global_pool.strings[id - 1];
    if (str == NULL)
        return NULL;

    if (len != NULL)
        *len = strlen(str);

    return str;
}

int intern_init(void) {
    if (global_pool_initialized)
        return 0;

    global_pool.arena = arena_init(ARENA_DEFAULT_CAP);

    if (global_pool.arena.base == NULL)
        return -1;

    uint32_t initial_cap = INTERN_TABLE_INIT_CAP;
    global_pool.table.hashes = arena_alloc(
        &global_pool.arena,
        initial_cap * sizeof(uint64_t)
    );
    global_pool.table.ids = arena_alloc(
        &global_pool.arena,
        initial_cap * sizeof(AtomId)
    );

    if (global_pool.table.hashes == NULL || global_pool.table.ids == NULL)
        return -1;

    for (uint32_t i = 0; i < initial_cap; i++) {
        global_pool.table.hashes[i] = 0;
        global_pool.table.ids[i]    = 0;
    }

    global_pool.table.count = 0;
    global_pool.table.cap = initial_cap;
    global_pool.strings     = NULL;
    global_pool.strings_cap = 0;
    global_pool.ref_count   = 0;
    global_pool_initialized = 1;
    return 0;
}

AtomId intern_string(const char *str, size_t len) {
    if (!global_pool_initialized)
        return 0;

    uint64_t h = hash(str, len);

    AtomId id = intern_lookup_hash(h, str, len);
    if (id != 0)
        return id;

    if (global_pool.table.count >= global_pool.table.cap / INTERN_TABLE_MAX_LOAD)
        if (table_grow() != 0)
            return 0;

    id = intern_assign_id(str, len, h);
    if (id == 0)
        return 0;

    return id;
}

int intern_equal(AtomId a, AtomId b) {
    return a == b;
}

void intern_retain(void) {
    if (!global_pool_initialized)
        return;
    global_pool.ref_count++;
}

void intern_release(void) {
    if (!global_pool_initialized)
        return;

    if (global_pool.ref_count > 0)
        global_pool.ref_count--;

    if (global_pool.ref_count == 0) {
        arena_free(&global_pool.arena);
        global_pool = (InternPool){0};
        global_pool_initialized = 0;
    }
}