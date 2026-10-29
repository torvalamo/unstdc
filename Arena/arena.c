#include "arena.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/types.h>

#if defined(_WIN32) || defined(_WIN64)
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
#else
    #include <unistd.h>
#endif

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

struct ArenaChunk {
    uint8_t *buffer;
    size_t capacity;
    size_t offset;
    struct ArenaChunk *next;
};

struct Arena {
    struct ArenaChunk *head;
    struct ArenaChunk *tail;
};

static struct ArenaChunk *ArenaChunk_Initialize(size_t capacity) {
    struct ArenaChunk *chunk = (struct ArenaChunk *)malloc(sizeof(struct ArenaChunk));
    if (!chunk) return NULL;

    size_t rounded_capacity = GetNearestBlockSize(capacity);
    chunk->buffer = (uint8_t *)malloc(rounded_capacity);
    if (!chunk->buffer) {
        free(chunk);
        return NULL;
    }
    chunk->capacity = rounded_capacity;
    chunk->offset = 0;
    chunk->next = NULL;
    return chunk;
}

static void ArenaChunk_Destroy(struct ArenaChunk *self) {
    if (self) {
        if (self->buffer) {
            free(self->buffer);
            self->buffer = NULL;
        }
        free(self);
    }
}

Arena *Arena_Initialize() {
    struct ArenaChunk *head = ArenaChunk_Initialize(0);
    if (!head) return NULL;
    Arena *arena = (Arena *)malloc(sizeof(Arena));
    if (!arena) {
        ArenaChunk_Destroy(head);
        return NULL;
    }
    arena->head = head;
    arena->tail = head;
    return arena;
}

void Arena_Destroy(Arena *self) {
    if (self) {
        struct ArenaChunk *curr = self->head;
        while (curr) {
            struct ArenaChunk *next = curr->next;
            ArenaChunk_Destroy(curr);
            curr = next;
        }
        free(self);
    }
}

void *Arena_Allocate(Arena *self, size_t size) {
    if (!self || size == 0) return NULL;

    size_t aligned_size = (size + 7) & ~7;

    struct ArenaChunk *chunk = self->tail;
    if (chunk->offset + aligned_size <= chunk->capacity) {
        void *memory = chunk->buffer + chunk->offset;
        chunk->offset += aligned_size;
        return memory;
    }

    struct ArenaChunk *new_chunk = ArenaChunk_Initialize(aligned_size);
    if (!new_chunk) return NULL;

    self->tail->next = new_chunk;
    self->tail = new_chunk;

    void *memory = new_chunk->buffer;
    new_chunk->offset = aligned_size;
    return memory;
}

void Arena_Reset(Arena *self) {
    if (self) {
        struct ArenaChunk *chunk = self->head->next;
        while (chunk) {
            struct ArenaChunk *next = chunk->next;
            ArenaChunk_Destroy(chunk);
            chunk = next;
        }
        self->head->offset = 0;
        self->head->next = NULL;
        self->tail = self->head;
    }
}

char *Arena_CopyString(Arena *self, const char *str) {
    if (!self || !str) return NULL;

    char *copy = ARENA_ALLOC_STRING(self, str);
    if (!copy) return NULL;
    memcpy(copy, str, strlen(str));
    copy[strlen(str)] = '\0';

    return copy;
}