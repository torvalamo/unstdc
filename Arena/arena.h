#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#if defined(_WIN32) && defined(ARENA_SHARED)
    #ifdef ARENA_BUILD
        #define ARENA_API __declspec(dllexport)
    #else
        #define ARENA_API __declspec(dllimport)
    #endif
#elif defined(__GNUC__) || defined(__clang__)
    #define ARENA_API __attribute__((visibility("default")))
#else
    #define ARENA_API
#endif

typedef struct Arena Arena;

typedef enum {
    // Arena allocates its chunks on the heap
    ARENA_BACKING_HEAP    = 0,
    // Arena allocates its chunks using virtual memory
    ARENA_BACKING_VIRTUAL = 1 << 0,

    // Arena grows dynamically
    ARENA_GROWTH_DYNAMIC = 0,
    // Arena has a finite size
    ARENA_GROWTH_FIXED   = 1 << 1,

    // Allocations are heterogenous (linear)
    ARENA_LAYOUT_LINEAR  = 0,
    // Allocations are pooled (fixed-size slots) and may reuse freed slots
    ARENA_LAYOUT_POOLED  = 1 << 2,

    // Arena frees its memory in bulk when destroyed, or may reuse free slots in a pooled layout
    ARENA_LIFETIME_BULK  = 0,
    // Arena reuses memory in a stack-like manner (LIFO)
    ARENA_LIFETIME_STACK = 1 << 3,
#ifdef ARENA_NO_PREFIX
    // Arena allocates its chunks on the heap
    BACKING_HEAP         = 0,
    // Arena allocates its chunks using virtual memory
    BACKING_VIRTUAL      = 1 << 0,

    // Arena grows dynamically
    GROWTH_DYNAMIC       = 0,
    // Arena has a finite size
    GROWTH_FIXED         = 1 << 1,

    // Allocations are heterogenous (linear)
    LAYOUT_LINEAR        = 0,
    // Allocations are pooled (fixed-size slots) and may reuse freed slots
    LAYOUT_POOLED        = 1 << 2,

    // Arena frees its memory in bulk when destroyed, or may reuse free slots in a pooled layout
    LIFETIME_BULK        = 0,
    // Arena reuses memory in a stack-like manner (LIFO)
    LIFETIME_STACK       = 1 << 3,
#endif
} ArenaFlags;

/**
 * Retrieves the last error message.
 *
 * @return A string describing the last error, or NULL if no error occurred.
 * @note Reading the string resets the error state.
 */
ARENA_API const char* Arena_Error(void);

/**
 * Initializes a new arena with the specified parent, flags, capacity, and slot size.
 *
 * @param parent The parent arena, or NULL for a top-level arena.
 * @param flags The arena flags.
 * @param capacity The initial capacity of the arena.
 * @param slotSize The size of each slot (for pooled arenas).
 * @return The initialized arena, or NULL on failure.
 */
ARENA_API Arena* Arena_InitializeWithParentAndSlotSize(Arena* parent, uint32_t flags, size_t capacity, size_t slotSize);

/**
 * Initializes a new arena with the specified parent, flags, and capacity.
 *
 * @param parent The parent arena, or NULL for a top-level arena.
 * @param flags The arena flags.
 * @param capacity The initial capacity of the arena.
 * @return The initialized arena, or NULL on failure.
 */
ARENA_API Arena* Arena_InitializeWithParent(Arena* parent, uint32_t flags, size_t capacity);

/**
 * Initializes a new arena with the specified flags, capacity, and slot size.
 *
 * @param flags The arena flags.
 * @param capacity The initial capacity of the arena.
 * @param slotSize The size of each slot (for pooled arenas).
 * @return The initialized arena, or NULL on failure.
 */
ARENA_API Arena* Arena_InitializeWithSlotSize(uint32_t flags, size_t capacity, size_t slotSize);

/**
 * Initializes a new arena with the specified flags and capacity.
 *
 * @param flags The arena flags.
 * @param capacity The initial capacity of the arena.
 * @return The initialized arena, or NULL on failure.
 */
ARENA_API Arena* Arena_Initialize(uint32_t flags, size_t capacity);

/**
 * Destroys the arena and frees all associated memory.
 *
 * @param self The arena to destroy.
 */
ARENA_API void Arena_Destroy(Arena* self);

/**
 * Allocates memory from the arena.
 *
 * @param self The arena to allocate from.
 * @param size The size of the memory to allocate.
 * @return A pointer to the allocated memory, or NULL on failure.
 * @note This function does not zero-initialize the memory.
 */
ARENA_API void* Arena_Allocate(Arena* self, size_t size);

/**
 * Allocates zero-initialized memory from the arena.
 *
 * @param self The arena to allocate from.
 * @param size The size of the memory to allocate.
 * @return A pointer to the allocated memory, or NULL on failure.
 * @note The allocated memory is guaranteed to be zero-initialized.
 */
ARENA_API void* Arena_AllocateZ(Arena* self, size_t size);

/**
 * Allocates memory for an array of instances of a type from the arena.
 *
 * @param self The arena to allocate from.
 * @param count The number of instances to allocate.
 * @param size The size of each instance.
 * @return A pointer to the allocated memory, or NULL on failure.
 */
ARENA_API void* Arena_AllocateArray(Arena* self, size_t count, size_t size);

/**
 * Allocates zero-initialized memory for an array of instances of a type from the arena.
 *
 * @param self The arena to allocate from.
 * @param count The number of instances to allocate.
 * @param size The size of each instance.
 * @return A pointer to the allocated memory, or NULL on failure.
 */
ARENA_API void* Arena_AllocateArrayZ(Arena* self, size_t count, size_t size);

/**
 * Frees a previously allocated memory block from the arena if the arena type allows it.
 *
 * @param self The arena to free from.
 * @param ptr The pointer to the memory block to free.
 * @return true if the memory was successfully freed, false otherwise.
 * @note This function may not have any effect if the arena does not support freeing individual allocations.
 */
ARENA_API bool Arena_Free(Arena* self, void* ptr);

/**
 * Pops the most recently allocated memory block from the arena if the arena type allows it.
 *
 * @param self The arena to pop from.
 * @return true if the most recent memory block was successfully popped, false otherwise.
 * @note This function may not have any effect if the arena does not support stack-like memory reuse.
 */
ARENA_API bool Arena_Pop(Arena* self);

/**
 * Resets the arena and frees any expanded chunks.
 *
 * @warning Ensure all requested memory is no longer in use before calling this function.
 *
 * @param self The arena to reset.
 */
ARENA_API void Arena_Reset(Arena* self);

/**
 * Copies a string into the arena.
 *
 * @param self The arena to allocate from.
 * @param str The (null-terminated) string to copy.
 * @return A pointer to the copied string, or NULL on failure.
 */
ARENA_API char* Arena_CopyString(Arena* self, const char* str);

/**
 * Copies raw data into the arena.
 *
 * @param self The arena to allocate from.
 * @param data The data to copy.
 * @param size The size of the data to copy.
 * @return A pointer to the copied data, or NULL on failure.
 */
ARENA_API void* Arena_CopyData(Arena* self, const void* data, size_t size);

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
#define ARENA_ALLOC_ARRAY(arena, count, type) ((type *)Arena_AllocateArray((arena), (count), sizeof(type)))

/**
 * Utility macro to allocate zero-initialized memory for an array of instances of a type from the arena.
 *
 * @param arena The arena to allocate from.
 * @param type The type of the memory to allocate.
 * @param count The number of instances to allocate.
 * @return A pointer to the allocated memory, or NULL on failure.
 */
#define ARENA_ALLOC_ARRAY_Z(arena, count, type) ((type *)Arena_AllocateArrayZ((arena), (count), sizeof(type)))

#ifdef ARENA_NO_PREFIX
#define ALLOC ARENA_ALLOC
#define ALLOC_Z ARENA_ALLOC_Z
#define ALLOC_ARRAY ARENA_ALLOC_ARRAY
#define ALLOC_ARRAY_Z ARENA_ALLOC_ARRAY_Z
#endif

/**
 * Retrieves the total memory used by the arena, including internal overhead.
 *
 * @param self The arena to query.
 * @return The total memory used by the arena.
 */
ARENA_API size_t Arena_GetUsed(const Arena* self);

/**
 * Retrieves the memory used by the arena for user allocations, excluding internal overhead.
 *
 * @param self The arena to query.
 * @return The memory used by the arena for user allocations.
 */
ARENA_API size_t Arena_GetUserUsed(const Arena* self);

/**
 * Retrieves the unused memory in the arena.
 *
 * @param self The arena to query.
 * @return The unused memory in the arena.
 */
ARENA_API size_t Arena_GetUnused(const Arena* self);

/**
 * Retrieves the available memory in the current tail chunk of the arena.
 *
 * @param self The arena to query.
 * @return The available memory in the current tail chunk.
 */
ARENA_API size_t Arena_GetAvailable(const Arena* self);

/**
 * Retrieves the total capacity of the arena, including internal overhead.
 *
 * @param self The arena to query.
 * @return The total capacity of the arena.
 */
ARENA_API size_t Arena_GetCapacity(const Arena* self);

/**
 * Retrieves the user-available capacity of the arena, excluding internal overhead.
 *
 * @param self The arena to query.
 * @return The user-available capacity of the arena.
 */
ARENA_API size_t Arena_GetUserCapacity(const Arena* self);

/**
 * Retrieves the committed virtual memory of the arena.
 *
 * @param self The arena to query.
 * @return The committed virtual memory of the arena.
 * @note This is only relevant for arenas using virtual memory.
 */
ARENA_API size_t Arena_GetCommitted(const Arena* self);