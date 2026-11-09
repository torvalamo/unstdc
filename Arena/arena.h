#pragma once

#include <stddef.h>
#include <stdint.h>

typedef struct Arena Arena;

/**
 * Initializes a new arena with a specified initial capacity.
 *
 * @param capacity The initial capacity of the arena.
 * @return The initialized arena, or NULL on failure.
 */
Arena *Arena_InitializeWithCapacity(size_t capacity);

/**
 * Initializes a new arena.
 *
 * @return The initialized arena.
 */
Arena *Arena_Initialize(void);

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
 * Allocates zero-initialized memory from the arena.
 *
 * @param self The arena to allocate from.
 * @param size The size of the memory to allocate.
 * @return A pointer to the allocated memory, or NULL on failure.
 */
void *Arena_AllocateZ(Arena *self, size_t size);

/**
 * Allocates memory for an array of instances of a type from the arena.
 *
 * @param self The arena to allocate from.
 * @param size The size of each instance.
 * @param count The number of instances to allocate.
 * @return A pointer to the allocated memory, or NULL on failure.
 */
void *Arena_AllocateArray(Arena *self, size_t size, size_t count);

/**
 * Allocates zero-initialized memory for an array of instances of a type from the arena.
 *
 * @param self The arena to allocate from.
 * @param size The size of each instance.
 * @param count The number of instances to allocate.
 * @return A pointer to the allocated memory, or NULL on failure.
 */
void *Arena_AllocateArrayZ(Arena *self, size_t size, size_t count);

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
 * Copies raw data into the arena.
 *
 * @param self The arena to allocate from.
 * @param data The data to copy.
 * @param size The size of the data to copy.
 * @return A pointer to the copied data, or NULL on failure.
 */
void *Arena_CopyData(Arena *self, const void *data, size_t size);

/**
 * Utility macro to allocate memory for a single instance of a type from the arena.
 *
 * @param arena The arena to allocate from.
 * @param type The type of the memory to allocate.
 * @return A pointer to the allocated memory, or NULL on failure.
 */
#define ARENA_ALLOC(arena, type) ((type *)Arena_Allocate((arena), sizeof(type)))

/**
 * Utility macro to allocate zero-initialized memory for a single instance of a type from the arena.
 *
 * @param arena The arena to allocate from.
 * @param type The type of the memory to allocate.
 * @return A pointer to the allocated memory, or NULL on failure.
 */
#define ARENA_ALLOC_Z(arena, type) ((type *)Arena_AllocateZ((arena), sizeof(type)))

/**
 * Utility macro to allocate memory for an array of instances of a type from the arena.
 *
 * @param arena The arena to allocate from.
 * @param type The type of the memory to allocate.
 * @param count The number of instances to allocate.
 * @return A pointer to the allocated memory, or NULL on failure.
 */
#define ARENA_ALLOC_ARRAY(arena, type, count) ((type *)Arena_AllocateArray((arena), sizeof(type), (count)))

/**
 * Utility macro to allocate zero-initialized memory for an array of instances of a type from the arena.
 *
 * @param arena The arena to allocate from.
 * @param type The type of the memory to allocate.
 * @param count The number of instances to allocate.
 * @return A pointer to the allocated memory, or NULL on failure.
 */
#define ARENA_ALLOC_ARRAY_Z(arena, type, count) ((type *)Arena_AllocateArrayZ((arena), sizeof(type), (count)))

/**
 * Retrieves the total memory used by the arena, including internal overhead.
 *
 * @param self The arena to query.
 * @return The total memory used by the arena.
 */
size_t Arena_GetUsed(const Arena *self);

/**
 * Retrieves the memory used by the arena for user allocations, excluding internal overhead.
 *
 * @param self The arena to query.
 * @return The memory used by the arena for user allocations.
 */
size_t Arena_GetUserUsed(const Arena *self);

/**
 * Retrieves the unused memory in the arena.
 *
 * @param self The arena to query.
 * @return The unused memory in the arena.
 */
size_t Arena_GetUnused(const Arena *self);

/**
 * Retrieves the available memory in the current tail chunk of the arena.
 *
 * @param self The arena to query.
 * @return The available memory in the current tail chunk.
 */
size_t Arena_GetAvailable(const Arena *self);

/**
 * Retrieves the total capacity of the arena, including internal overhead.
 *
 * @param self The arena to query.
 * @return The total capacity of the arena.
 */
size_t Arena_GetCapacity(const Arena *self);

/**
 * Retrieves the user-available capacity of the arena, excluding internal overhead.
 *
 * @param self The arena to query.
 * @return The user-available capacity of the arena.
 */
size_t Arena_GetUserCapacity(const Arena *self);