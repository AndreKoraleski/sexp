#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "arena.h"

#define ALIGN_UP(n) \
    (((n) + alignof(max_align_t) - 1) & ~(alignof(max_align_t) - 1))

Arena arena_init(size_t cap) {
    if (cap == 0) cap = ARENA_DEFAULT_CAP;
    Arena arena = {0};
    arena.base = malloc(cap);
    if (!arena.base) {
        return arena; /* zeroed struct indicates failure */
    }
    arena.cap = cap;
    return arena;
}

void *arena_alloc(Arena *arena, size_t size) {
    size_t aligned = ALIGN_UP(size);
    if (arena->pos + aligned > arena->cap) {
        size_t new_cap = arena->cap << 1;
        if (new_cap < aligned) {
            new_cap = aligned;
        }
        Arena *chunk = malloc(sizeof(Arena));
        if (!chunk) {
            return NULL;
        }
        *chunk       = *arena;
        uint8_t *new_base = malloc(new_cap);
        if (!new_base) {
            free(chunk);
            return NULL;
        }
        arena->base = new_base;
        arena->cap  = new_cap;
        arena->pos  = 0;
        arena->prev = chunk;
    }
    void *ptr    = arena->base + arena->pos;
    arena->pos  += aligned;
    return ptr;
}
void arena_reset(Arena *arena) {
    /* Walk to the oldest chunk (prev == NULL), freeing everything newer. */
    Arena *chunk = arena->prev;
    while (chunk) {
        Arena *prev = chunk->prev;
        if (!prev) {
            /* This is the first chunk; restore arena to it and stop. */
            arena->base = chunk->base;
            arena->cap  = chunk->cap;
            arena->pos  = 0;
            arena->prev = NULL;
            free(chunk);
            return;
        }
        free(chunk->base);
        free(chunk);
        chunk = prev;
    }
    arena->pos = 0;
}

void arena_free(Arena *arena) {
    /* Free all previous chunks and their backing buffers. */
    Arena *chunk = arena->prev;
    while (chunk) {
        Arena *prev = chunk->prev;
        free(chunk->base);
        free(chunk);
        chunk = prev;
    }
    free(arena->base);
    *arena = (Arena){0};
}
