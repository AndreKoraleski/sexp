#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "memory/arena.h"


/**
 * @brief Round n up to the next multiple of max_align_t, ensuring all returned pointers are
 * suitably aligned for any type.
 */
#define ALIGN_UP(n) (((n) + alignof(max_align_t) - 1) & ~(alignof(max_align_t) - 1))

Arena arena_init(size_t capacity) {
    Arena arena = {0};

    if (capacity == 0) {
        capacity = ARENA_DEFAULT_CAPACITY;
    }

    arena.capacity = capacity;
    arena.base     = malloc(capacity);

    if (arena.base == NULL) {
        arena.capacity = 0;
    }

    return arena;
}

void *arena_alloc(Arena *arena, size_t size) {
    size_t aligned = ALIGN_UP(size);

    if (arena->position + aligned > arena->capacity) {
        size_t new_capacity = arena->capacity << 1;

        /* Ensure the new chunk is large enough to satisfy this single allocation. */
        if (new_capacity < aligned) {
            new_capacity = aligned;
        }

        Arena *chunk = malloc(sizeof(Arena));
        if (chunk == NULL) {
            return NULL;
        }

        uint8_t *new_buffer = malloc(new_capacity);
        if (new_buffer == NULL) {
            free(chunk);
            return NULL;
        }

        *chunk = *arena;

        arena->base           = new_buffer;
        arena->capacity       = new_capacity;
        arena->position       = 0;
        arena->previous_chunk = chunk;
    }

    void *pointer = arena->base + arena->position;
    arena->position += aligned;

    return pointer;
}

void arena_reset(Arena *arena) {
    Arena *chunk = arena->previous_chunk;

    if (chunk == NULL) {
        /* Only one chunk - just rewind the cursor. */
        arena->position = 0;
        return;
    }

    /* Walk back to the very first chunk, freeing every intermediate one. */
    while (chunk->previous_chunk != NULL) {
        Arena *previous_chunk = chunk->previous_chunk;

        free(chunk->base);
        free(chunk);

        chunk = previous_chunk;
    }

    /* Adopt the oldest surviving chunk as the current one and rewind it. */
    free(arena->base);
    arena->base           = chunk->base;
    arena->capacity       = chunk->capacity;
    arena->position       = 0;
    arena->previous_chunk = NULL;

    free(chunk);
}

void arena_free(Arena *arena) {
    Arena *chunk = arena->previous_chunk;

    while (chunk != NULL) {
        Arena *previous_chunk = chunk->previous_chunk;

        free(chunk->base);
        free(chunk);

        chunk = previous_chunk;
    }

    free(arena->base);

    *arena = (Arena){0};
}
