#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "arena.h"

#define ALIGN_UP(n) \
    (((n) + alignof(max_align_t) - 1) & ~(alignof(max_align_t) - 1))

Arena arena_init(size_t cap) {
    Arena arena = {0};

    if (cap == 0) 
        cap = ARENA_DEFAULT_CAP;

    arena.cap = cap;
    arena.base = malloc(cap);

    if (arena.base == NULL) 
        arena.cap = 0;

    return arena;
}

void *arena_alloc(Arena *arena, size_t size) {
    size_t aligned = ALIGN_UP(size);

    /* Grow arena if necessary */
    if (arena->pos + aligned > arena->cap) {
        size_t new_cap = arena->cap << 1;

        if (new_cap < aligned) 
            new_cap = aligned;

        Arena *chunk = malloc(sizeof(Arena));
        if (chunk == NULL) 
            return NULL;

        uint8_t *new_base = malloc(new_cap);
        if (new_base == NULL) {
            free(chunk);
            return NULL;
        }

        *chunk = *arena;

        arena->base = new_base;
        arena->cap  = new_cap;
        arena->pos  = 0;
        arena->prev = chunk;
    }

    void *ptr = arena->base + arena->pos;
    arena->pos += aligned;

    return ptr;
}

void arena_reset(Arena *arena) {
    /* Walk to the oldest chunk (prev == NULL), freeing everything newer. */
    Arena *chunk = arena->prev;

    if (chunk == NULL) {
        arena->pos = 0;
        return;
    }

    while (chunk->prev != NULL) {
        Arena *prev = chunk->prev;

        free(chunk->base);
        free(chunk);

        chunk = prev;
    }

    /* chunk now points to the oldest one */
    free(arena->base);
    arena->base = chunk->base;
    arena->cap  = chunk->cap;
    arena->pos  = 0;
    arena->prev = NULL;

    free(chunk);
}

void arena_free(Arena *arena) {
    /* Free chained chunks */
    Arena *chunk = arena->prev;

    while (chunk != NULL) {
        Arena *prev = chunk->prev;

        free(chunk->base);
        free(chunk);

        chunk = prev;
    }

    free(arena->base);

    *arena = (Arena){0};
}
