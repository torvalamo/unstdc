#include "arena.h"

#include <stdalign.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
#else
    #include <unistd.h>
#endif

struct Arena_ArenaChunk {
    uint8_t *buffer;
    size_t capacity;
    size_t offset;
    struct Arena_ArenaChunk *next;
};

typedef struct Arena {
    struct Arena_ArenaChunk *head;
    struct Arena_ArenaChunk *tail;
} Arena;

static const size_t STRUCT_ARENA_SIZE = (sizeof(Arena) + alignof(max_align_t) - 1) & ~(alignof(max_align_t) - 1);
static const size_t STRUCT_CHUNK_SIZE = (sizeof(struct Arena_ArenaChunk) + alignof(max_align_t) - 1) & ~(alignof(max_align_t) - 1);

static size_t GetSystemPageSize() {
    static size_t page_size = 0;

    if (page_size == 0) {
        #if defined(_WIN32) || defined(_WIN64)
            SYSTEM_INFO sys_info;
            GetSystemInfo(&sys_info);
            page_size = (size_t)sys_info.dwPageSize;
        #else
            // Works flawlessly on Linux, macOS, iOS, Android, and WebAssembly (Emscripten)
            #if defined(_SC_PAGESIZE)
                long result = sysconf(_SC_PAGESIZE);
            #elif defined(_SC_PAGE_SIZE)
                long result = sysconf(_SC_PAGE_SIZE);
            #else
                long result = -1;
            #endif

            if (result > 0) {
                page_size = (size_t)result;
            }
        #endif
        if (page_size == 0) {
            page_size = 4096; // Fallback to default page size for unknown OS
        }
    }

    return page_size;
}

static uint8_t GetSystemPageBitShift() {
    static uint8_t page_bit_shift = 0;

    if (page_bit_shift == 0) {
        size_t page_size = GetSystemPageSize();
        while (page_size > 1) {
            page_bit_shift++;
            page_size >>= 1;
        }
    }

    return page_bit_shift;
}
static size_t GetNearestBlockSize(size_t value) {
    if (value == 0) return GetSystemPageSize();

    size_t max_safe_value = SIZE_MAX - (GetSystemPageSize() - 1);
    if (value > max_safe_value) return 0;

    return (((value - 1) >> GetSystemPageBitShift()) + 1) * GetSystemPageSize();
}

static inline size_t AlignUp(size_t size, size_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

static struct Arena_ArenaChunk *ArenaChunk_Initialize(size_t capacity) {
    if (capacity > SIZE_MAX - STRUCT_CHUNK_SIZE) return NULL;
    size_t rounded_capacity = GetNearestBlockSize(capacity + STRUCT_CHUNK_SIZE);
    if (rounded_capacity == 0) return NULL;
    
    struct Arena_ArenaChunk *chunk = (struct Arena_ArenaChunk *)malloc(rounded_capacity);
    if (!chunk) return NULL;

    chunk->buffer = (uint8_t *)chunk;
    chunk->capacity = rounded_capacity;
    chunk->offset = STRUCT_CHUNK_SIZE;
    chunk->next = NULL;
    
    return chunk;
}

static void ArenaChunk_Destroy(struct Arena_ArenaChunk *self) {
    if (self && self->buffer) free(self->buffer);
}

Arena *Arena_InitializeWithCapacity(size_t capacity) {
    if (capacity > SIZE_MAX - STRUCT_ARENA_SIZE) return NULL;
    struct Arena_ArenaChunk *head = ArenaChunk_Initialize(capacity + STRUCT_ARENA_SIZE);
    if (!head) return NULL;
    Arena *arena = (Arena *)(head->buffer + STRUCT_CHUNK_SIZE);
    arena->head = head;
    arena->tail = head;
    head->offset += STRUCT_ARENA_SIZE;
    return arena;
}

Arena *Arena_Initialize(void) {
    return Arena_InitializeWithCapacity(0);
}

void Arena_Destroy(Arena *self) {
    if (!self) return;
    struct Arena_ArenaChunk *current = self->head;
    while (current) {
        struct Arena_ArenaChunk *next = current->next;
        ArenaChunk_Destroy(current);
        current = next;
    }
}

void *Arena_Allocate(Arena *self, size_t size) {
    if (!self || size == 0) return NULL;
    if (size > SIZE_MAX - (alignof(max_align_t) - 1)) return NULL;

    size_t aligned_size = AlignUp(size, alignof(max_align_t));

    struct Arena_ArenaChunk *tail = self->tail;
    if (aligned_size <= tail->capacity - tail->offset) {
        void *memory = tail->buffer + tail->offset;
        tail->offset += aligned_size;
        return memory;
    }

    struct Arena_ArenaChunk *new_chunk = ArenaChunk_Initialize(aligned_size);
    if (!new_chunk) return NULL;

    tail->next = new_chunk;
    self->tail = new_chunk;

    void *memory = new_chunk->buffer + new_chunk->offset;
    new_chunk->offset += aligned_size;
    return memory;
}

void *Arena_AllocateZ(Arena *self, size_t size) {
    void *memory = Arena_Allocate(self, size);
    if (memory) memset(memory, 0, size);
    return memory;
}

void *Arena_AllocateArray(Arena *self, size_t size, size_t count) {
    if (!self || size == 0 || count == 0) return NULL;
    if (count > SIZE_MAX / size) return NULL;
    return Arena_Allocate(self, size * count);
}

void *Arena_AllocateArrayZ(Arena *self, size_t size, size_t count) {
    if (!self || size == 0 || count == 0) return NULL;
    if (count > SIZE_MAX / size) return NULL;
    return Arena_AllocateZ(self, size * count);
}

void Arena_Reset(Arena *self) {
    if (!self || !self->head) return;

    struct Arena_ArenaChunk *current = self->head->next;
    while (current) {
        struct Arena_ArenaChunk *next = current->next;
        ArenaChunk_Destroy(current);
        current = next;
    }
    
    // Preserve the initial chunk's offset to account for the Chunk and Arena structs themselves.
    self->head->offset = STRUCT_CHUNK_SIZE + STRUCT_ARENA_SIZE;
    self->head->next = NULL;
    self->tail = self->head;
}

char *Arena_CopyString(Arena *self, const char *str) {
    if (!self || !str) return NULL;

    size_t len = strlen(str);
    char *copy = ((char *)Arena_Allocate(self, len + 1));
    if (!copy) return NULL;

    memcpy(copy, str, len);
    copy[len] = '\0';

    return copy;
}

void *Arena_CopyData(Arena *self, const void *data, size_t size) {
    if (!self || !data || size == 0) return NULL;

    void *copy = Arena_Allocate(self, size);
    if (!copy) return NULL;
    memcpy(copy, data, size);

    return copy;
}

size_t Arena_GetUsed(const Arena *self) {
    if (!self || !self->head) return 0;
    size_t used = 0;
    struct Arena_ArenaChunk *current = self->head;
    while (current) {
        used += current->offset;
        current = current->next;
    }
    return used;
}

size_t Arena_GetUserUsed(const Arena *self) {
    if (!self || !self->head) return 0;
    size_t used = 0;
    struct Arena_ArenaChunk *current = self->head;
    while (current) {
        used += current->offset - STRUCT_CHUNK_SIZE;
        current = current->next;
    }
    return used - STRUCT_ARENA_SIZE;
}

size_t Arena_GetUnused(const Arena *self) {
    if (!self || !self->head) return 0;
    size_t unused = 0;
    struct Arena_ArenaChunk *current = self->head;
    while (current) {
        unused += current->capacity - current->offset;
        current = current->next;
    }
    return unused;
}

size_t Arena_GetAvailable(const Arena *self) {
    if (!self || !self->tail) return 0;
    return self->tail->capacity - self->tail->offset;
}

size_t Arena_GetCapacity(const Arena *self) {
    if (!self || !self->head) return 0;
    size_t capacity = 0;
    struct Arena_ArenaChunk *current = self->head;
    while (current) {
        capacity += current->capacity;
        current = current->next;
    }
    return capacity;
}

size_t Arena_GetUserCapacity(const Arena *self) {
    if (!self || !self->head) return 0;
    size_t capacity = 0;
    struct Arena_ArenaChunk *current = self->head;
    while (current) {
        capacity += current->capacity - STRUCT_CHUNK_SIZE;
        current = current->next;
    }
    return capacity - STRUCT_ARENA_SIZE;
}