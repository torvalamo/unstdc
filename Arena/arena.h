#pragma once

#include <stddef.h>

typedef struct Arena Arena;

/**
 * Initializes a new arena.
 *
 * @return The initialized arena.
 */
Arena *Arena_Initialize();

/**
 * Destroys the arena and frees all associated memory.
 *
 * @param self The arena to destroy.
 */
void Arena_Destroy(Arena *self);

/**
 * Allocates memory from the arena.
 *
 * @param self The arena to allocate from.
 * @param size The size of the memory to allocate.
 * @return A pointer to the allocated memory, or NULL on failure.
 */
void *Arena_Allocate(Arena *self, size_t size);

/**
 * Resets the arena and frees any expanded chunks.
 *
 * @warning Ensure all requested memory is no longer in use before calling this function.
 *
 * @param self The arena to reset.
 */
void Arena_Reset(Arena *self);

/**
 * Copies a string into the arena.
 *
 * @param self The arena to allocate from.
 * @param str The (null-terminated) string to copy.
 * @return A pointer to the copied string, or NULL on failure.
 */
char *Arena_CopyString(Arena *self, const char *str);

/**
 * Allocates memory for a single instance of a type from the arena.
 *
 * @param arena The arena to allocate from.
 * @param type The type of the memory to allocate.
 * @return A pointer to the allocated memory, or NULL on failure.
 */
#define ARENA_ALLOC(arena, type) ((type *)Arena_Allocate((arena), sizeof(type)))

/**
 * Allocates memory for an array of instances of a type from the arena.
 *
 * @param arena The arena to allocate from.
 * @param type The type of the memory to allocate.
 * @param count The number of instances to allocate.
 * @return A pointer to the allocated memory, or NULL on failure.
 */
#define ARENA_ALLOC_ARRAY(arena, type, count) ((type *)Arena_Allocate((arena), sizeof(type) * (count)))

/**
 * Allocates memory for a string from the arena.
 *
 * @param arena The arena to allocate from.
 * @param str The string to allocate memory for.
 * @return A pointer to the allocated memory, or NULL on failure.
 */
#define ARENA_ALLOC_STRING(arena, str) ((char *)Arena_Allocate(arena, strlen(str) + 1))