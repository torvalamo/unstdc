#include "arena.h"

#include <stdalign.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
#else
    #include <unistd.h>
    #include <sys/mman.h>
#endif

#if defined(_MSC_VER)
    #define ARENA_THREAD_LOCAL __declspec(thread)
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_THREADS__)
    #define ARENA_THREAD_LOCAL _Thread_local
#elif defined(__GNUC__) || defined(__clang__)
    #define ARENA_THREAD_LOCAL __thread
#else
    #define ARENA_THREAD_LOCAL
#endif

typedef size_t stack_header_t;
typedef struct Chunk Chunk;

struct Chunk {
    uint8_t* buffer;
    size_t capacity;
    size_t offset;
    Chunk* next;
};

struct Arena {
    Chunk* head;
    Chunk* tail;
    Arena* parent;
    size_t committed;
    size_t defaultChunkSize;
    size_t slotSize;
    void* freeList;
    uint32_t flags;
};

static const size_t STRUCT_ARENA_SIZE = (sizeof(Arena) + alignof(max_align_t) - 1) & ~(alignof(max_align_t) - 1);
static const size_t STRUCT_CHUNK_SIZE = (sizeof(Chunk) + alignof(max_align_t) - 1) & ~(alignof(max_align_t) - 1);
static const size_t STACK_HEADER_SIZE = (sizeof(stack_header_t) + alignof(max_align_t) - 1) & ~(alignof(max_align_t) - 1);

static ARENA_THREAD_LOCAL const char* lastError = NULL;

#define ARENA_HAS_PARENT        (1u << 31)
#define ARENA_IS_CHILD(flags)   (((flags) & ARENA_HAS_PARENT) != 0)

#define ARENA_IS_VIRTUAL(flags) (((flags) & ARENA_BACKING_VIRTUAL) != 0 && !ARENA_IS_CHILD(flags))
#define ARENA_IS_HEAP(flags)    (((flags) & ARENA_BACKING_VIRTUAL) == 0 && !ARENA_IS_CHILD(flags))
#define ARENA_IS_FIXED(flags)   (((flags) & ARENA_GROWTH_FIXED) != 0)
#define ARENA_IS_DYNAMIC(flags) (((flags) & ARENA_GROWTH_FIXED) == 0)
#define ARENA_IS_POOLED(flags)  (((flags) & ARENA_LAYOUT_POOLED) != 0)
#define ARENA_IS_LINEAR(flags)  (((flags) & ARENA_LAYOUT_POOLED) == 0)
#define ARENA_IS_STACK(flags)   (((flags) & ARENA_LIFETIME_STACK) != 0)
#define ARENA_IS_BULK(flags)    (((flags) & ARENA_LIFETIME_STACK) == 0)

static void Chunk_Destroy(Chunk* chunk, uint32_t flags);
static size_t Chunk_BaseOffset(const Arena* self, const Chunk* chunk);
static bool Chunk_EnsureSpace(Arena* self, Chunk* chunk, size_t needed);
static Chunk* Chunk_Create(Arena* parent, uint32_t flags, size_t capacity, size_t* outCommitted);
static size_t GetSystemPageSize(void);
static size_t GetNearestBlockSize(size_t value);
static uint8_t GetSystemPageBitShift(void);
static inline size_t AlignUp(size_t size, size_t alignment);
static void* VMem_Reserve(size_t size);
static int VMem_Commit(void* addr, size_t size);
static void VMem_Decommit(void* addr, size_t size);
static void VMem_Release(void* addr, size_t reservedSize);
static size_t CountFreeSlots(const Arena* self);
static size_t CountStackFooters(const Arena* self);
#ifndef NDEBUG
static bool Pool_IsValidActiveSlot(const Arena* self, const void* ptr);
#endif

ARENA_API const char* Arena_Error(void) {
    const char* error = lastError;
    lastError = NULL;
    return error;
}

ARENA_API Arena* Arena_InitializeWithParentAndSlotSize(Arena* parent, uint32_t flags, size_t capacity, size_t slotSize) {
    if (parent != NULL) {
        if (flags & ARENA_BACKING_VIRTUAL) {
            lastError = "Virtual arenas cannot have a parent and must be top-level.";
            return NULL;
        }
        flags |= ARENA_HAS_PARENT;
    }

    size_t aligned_slot_size = 0;
    if (ARENA_IS_POOLED(flags)) {
        if (slotSize == 0) {
            lastError = "Slot size cannot be zero for pooled arenas.";
            return NULL;
        }
        if (slotSize > SIZE_MAX - (alignof(max_align_t) - 1)) {
            lastError = "Slot size is too large.";
            return NULL;
        }
        aligned_slot_size = AlignUp(slotSize, alignof(max_align_t));
    }

    if (capacity < aligned_slot_size) {
        capacity = aligned_slot_size;
    }
    if (capacity > SIZE_MAX - STRUCT_ARENA_SIZE) {
        lastError = "Capacity is too large.";
        return NULL;
    }

    size_t committed = 0;
    Chunk* head = Chunk_Create(parent, flags, capacity + STRUCT_ARENA_SIZE, &committed);
    if (!head) return NULL; // Error should be set in Chunk_Create

    Arena* arena = (Arena*)(head->buffer + STRUCT_CHUNK_SIZE);
    head->offset += STRUCT_ARENA_SIZE;

    arena->head = head;
    arena->tail = head;
    arena->parent = parent;
    arena->committed = committed;
    arena->defaultChunkSize = head->capacity - STRUCT_CHUNK_SIZE;
    arena->slotSize = aligned_slot_size;
    arena->freeList = NULL;
    arena->flags = flags;

    return arena;
}

ARENA_API Arena* Arena_InitializeWithParent(Arena* parent, uint32_t flags, size_t capacity) {
    return Arena_InitializeWithParentAndSlotSize(parent, flags, capacity, 0);
}

ARENA_API Arena* Arena_InitializeWithSlotSize(uint32_t flags, size_t capacity, size_t slotSize) {
    return Arena_InitializeWithParentAndSlotSize(NULL, flags, capacity, slotSize);
}

ARENA_API Arena* Arena_Initialize(uint32_t flags, size_t capacity) {
    return Arena_InitializeWithParentAndSlotSize(NULL, flags, capacity, 0);
}

ARENA_API void Arena_Destroy(Arena* self) {
    if (!self) return;

    uint32_t flags = self->flags;

    // Child arenas are carved out of a parent arena; the parent owns their memory
    if (ARENA_IS_CHILD(flags)) {
        return;
    }

    Chunk* current = self->head;
    while (current) {
        Chunk* next = current->next;
        Chunk_Destroy(current, flags);
        current = next;
    }
}

ARENA_API void* Arena_Allocate(Arena* self, size_t size) {
    if (!self) {
        lastError = "Arena cannot be NULL.";
        return NULL;
    }

    size_t alloc_size = 0;

    if (ARENA_IS_POOLED(self->flags)) {
        // Pooled + Bulk: reuse from intrusive free list if available
        if (ARENA_IS_BULK(self->flags) && self->freeList != NULL) {
            void* slot = self->freeList;
            self->freeList = *(void**)slot;
            return slot;
        }
        if (size > self->slotSize) {
            lastError = "Requested size exceeds pool slot size. Pass zero for default slot size.";
            return NULL;
        }
        alloc_size = self->slotSize;
    } else {
        if (size == 0) {
            lastError = "Allocation size cannot be zero in non-pooled arenas.";
            return NULL;
        }
        if (size > SIZE_MAX - (alignof(max_align_t) - 1)) {
            lastError = "Allocation size is too large.";
            return NULL;
        }
        alloc_size = AlignUp(size, alignof(max_align_t));

        // Linear + Stack: reserve trailing footer to record previous offset
        if (ARENA_IS_STACK(self->flags)) {
            if (alloc_size > SIZE_MAX - STACK_HEADER_SIZE) {
                lastError = "Allocation size is too large.";
                return NULL;
            }
            alloc_size += STACK_HEADER_SIZE;
        }
    }

    Chunk* tail = self->tail;

    // Try allocating from the current tail chunk
    if (alloc_size <= tail->capacity - tail->offset) {
        if (!Chunk_EnsureSpace(self, tail, alloc_size)) return NULL;

        size_t prev_offset = tail->offset;
        void* memory = tail->buffer + prev_offset;
        tail->offset += alloc_size;

        if (ARENA_IS_LINEAR(self->flags) && ARENA_IS_STACK(self->flags)) {
            stack_header_t* footer = (stack_header_t*)(tail->buffer + tail->offset - STACK_HEADER_SIZE);
            *footer = prev_offset;
        }
        return memory;
    }

    // Tail is full: fail if FIXED or VIRTUAL, otherwise allocate a new chunk
    if (ARENA_IS_FIXED(self->flags) || ARENA_IS_VIRTUAL(self->flags)) {
        lastError = "Arena has reached maximum capacity.";
        return NULL;
    }

    size_t next_chunk_cap = (alloc_size > self->defaultChunkSize) ? alloc_size : self->defaultChunkSize;
    Chunk* new_chunk = Chunk_Create(self->parent, self->flags, next_chunk_cap, NULL);
    if (!new_chunk) return NULL;

    tail->next = new_chunk;
    self->tail = new_chunk;

    size_t prev_offset = new_chunk->offset;
    void* memory = new_chunk->buffer + prev_offset;
    new_chunk->offset += alloc_size;

    if (ARENA_IS_LINEAR(self->flags) && ARENA_IS_STACK(self->flags)) {
        stack_header_t* footer = (stack_header_t*)(new_chunk->buffer + new_chunk->offset - STACK_HEADER_SIZE);
        *footer = prev_offset;
    }
    return memory;
}

ARENA_API void* Arena_AllocateZ(Arena* self, size_t size) {
    void* memory = Arena_Allocate(self, size);
    if (memory) {
        size_t zero_size = ARENA_IS_POOLED(self->flags) ? self->slotSize : size;
        memset(memory, 0, zero_size);
    }
    return memory;
}

ARENA_API void* Arena_AllocateArray(Arena* self, size_t count, size_t size) {
    if (ARENA_IS_POOLED(self->flags)) {
        lastError = "Array allocations are not supported on pooled arenas.";
        return NULL;
    }
    if (count == 0 || size == 0) {
        lastError = "Cannot allocate zero items or zero-sized items.";
        return NULL;
    }
    size_t total_size = size * count;
    if (size != 0 && total_size / size != count) {
        lastError = "Array allocation size overflow.";
        return NULL;
    }
    return Arena_Allocate(self, total_size);
}

ARENA_API void* Arena_AllocateArrayZ(Arena* self, size_t count, size_t size) {
    void* memory = Arena_AllocateArray(self, count, size);
    if (memory) {
        memset(memory, 0, size * count);
    }
    return memory;
}

ARENA_API bool Arena_Free(Arena* self, void* ptr) {
    if (!ptr) {
        lastError = "Pointer to free cannot be NULL.";
        return false;
    }

    if (ARENA_IS_POOLED(self->flags) && ARENA_IS_BULK(self->flags)) {
#ifndef NDEBUG
        if (!Pool_IsValidActiveSlot(self, ptr)) {
            return false; // lastError is set by Pool_IsValidActiveSlot
        }
#endif
        *(void**)ptr = self->freeList;
        self->freeList = ptr;
        return true;
    }

    if (ARENA_IS_STACK(self->flags)) {
        Chunk* tail = self->tail;
        size_t base_offset = Chunk_BaseOffset(self, tail);

        if (tail->offset <= base_offset) {
            lastError = "Cannot free from an empty stack arena.";
            return false;
        }

        void* expected_top = NULL;
        if (ARENA_IS_POOLED(self->flags)) {
            expected_top = tail->buffer + tail->offset - self->slotSize;
        } else {
            stack_header_t* footer = (stack_header_t*)(tail->buffer + tail->offset - STACK_HEADER_SIZE);
            expected_top = tail->buffer + *footer;
        }

        if (ptr != expected_top) {
            lastError = "Pointer is not at the top of the stack (LIFO violation).";
            return false;
        }

        return Arena_Pop(self);
    }

    lastError = "Arena_Free is not supported on linear bulk arenas.";
    return false;
}

ARENA_API bool Arena_Pop(Arena* self) {
    if (!ARENA_IS_STACK(self->flags)) {
        lastError = "Arena_Pop can only be called on stack arenas.";
        return false;
    }

    Chunk* tail = self->tail;
    size_t base_offset = Chunk_BaseOffset(self, tail);

    // If current tail chunk is empty, step back to the previous chunk (if dynamic)
    if (tail->offset <= base_offset) {
        if (tail == self->head) {
            lastError = "Stack arena is already empty.";
            return false;
        }

        Chunk* prev = self->head;
        while (prev && prev->next != tail) {
            prev = prev->next;
        }
        if (!prev) return true;

        Chunk_Destroy(tail, self->flags);
        prev->next = NULL;
        self->tail = prev;
        tail = prev;
        base_offset = Chunk_BaseOffset(self, tail);
    }

    if (ARENA_IS_POOLED(self->flags)) {
        // Homogeneous vector stack: roll back by slotSize
        if (tail->offset - base_offset < self->slotSize) {
            tail->offset = base_offset;
        } else {
            tail->offset -= self->slotSize;
        }
    } else {
        // Heterogeneous linear stack: roll back using the trailing footer
        if (tail->offset - base_offset < STACK_HEADER_SIZE) {
            tail->offset = base_offset;
        } else {
            stack_header_t* footer = (stack_header_t*)(tail->buffer + tail->offset - STACK_HEADER_SIZE);
            tail->offset = *footer;
        }
    }

    // If the pop completely emptied a non-head chunk, clean it up so tail always points to active data
    if (tail != self->head && tail->offset <= base_offset) {
        Chunk* prev = self->head;
        while (prev && prev->next != tail) {
            prev = prev->next;
        }
        if (prev) {
            Chunk_Destroy(tail, self->flags);
            prev->next = NULL;
            self->tail = prev;
        }
    }
    return true;
}

ARENA_API void Arena_Reset(Arena* self) {
    if (!self || !self->head) return;

    self->freeList = NULL;

    if (ARENA_IS_VIRTUAL(self->flags)) {
        size_t initial_commit = GetNearestBlockSize(STRUCT_CHUNK_SIZE + STRUCT_ARENA_SIZE);
        if (initial_commit > self->head->capacity) {
            initial_commit = self->head->capacity;
        }
        if (self->committed > initial_commit) {
            VMem_Decommit(self->head->buffer + initial_commit, self->committed - initial_commit);
            self->committed = initial_commit;
        }
    } else {
        Chunk* current = self->head->next;
        while (current) {
            Chunk* next = current->next;
            Chunk_Destroy(current, self->flags);
            current = next;
        }
        self->head->next = NULL;
    }

    self->head->offset = STRUCT_CHUNK_SIZE + STRUCT_ARENA_SIZE;
    self->tail = self->head;
}

ARENA_API char* Arena_CopyString(Arena* self, const char* str) {
    if (!self || !str) {
        lastError = "Arena or string cannot be NULL.";
        return NULL;
    }
    size_t len = strlen(str) + 1;
    char* copy = (char*)Arena_Allocate(self, len);
    if (copy) {
        memcpy(copy, str, len);
    }
    return copy;
}

ARENA_API void* Arena_CopyData(Arena* self, const void* data, size_t size) {
    if (!self || !data || size == 0) {
        lastError = "Arena, data or size cannot be NULL or zero.";
        return NULL;
    }
    void* copy = Arena_Allocate(self, size);
    if (copy) {
        memcpy(copy, data, size);
    }
    return copy;
}

ARENA_API size_t Arena_GetUsed(const Arena* self) {
    if (!self || !self->head) return 0;

    size_t used = 0;
    const Chunk* current = self->head;
    while (current) {
        used += current->offset;
        current = current->next;
    }

    // Subtract slots currently sitting idle in the pool free list
    used -= CountFreeSlots(self) * self->slotSize;
    return used;
}

ARENA_API size_t Arena_GetUserUsed(const Arena* self) {
    if (!self || !self->head) return 0;

    size_t used = 0;
    const Chunk* current = self->head;
    while (current) {
        used += current->offset - STRUCT_CHUNK_SIZE;
        current = current->next;
    }

    used -= STRUCT_ARENA_SIZE;
    used -= CountFreeSlots(self) * self->slotSize;
    used -= CountStackFooters(self) * STACK_HEADER_SIZE;
    return used;
}

ARENA_API size_t Arena_GetUnused(const Arena* self) {
    if (!self || !self->head) return 0;

    size_t unused = 0;
    const Chunk* current = self->head;
    while (current) {
        unused += current->capacity - current->offset;
        current = current->next;
    }

    // Free-listed slots are available for reuse
    unused += CountFreeSlots(self) * self->slotSize;
    return unused;
}

ARENA_API size_t Arena_GetAvailable(const Arena* self) {
    if (!self || !self->tail) return 0;

    size_t available = self->tail->capacity - self->tail->offset;

    // On a POOLED | BULK arena, any slot on the free list can be allocated immediately
    if (ARENA_IS_POOLED(self->flags) && ARENA_IS_BULK(self->flags)) {
        available += CountFreeSlots(self) * self->slotSize;
    }
    return available;
}

ARENA_API size_t Arena_GetCapacity(const Arena* self) {
    if (!self || !self->head) return 0;

    size_t capacity = 0;
    const Chunk* current = self->head;
    while (current) {
        capacity += current->capacity;
        current = current->next;
    }
    return capacity;
}

ARENA_API size_t Arena_GetUserCapacity(const Arena* self) {
    if (!self || !self->head) return 0;

    size_t capacity = 0;
    const Chunk* current = self->head;
    while (current) {
        capacity += current->capacity - STRUCT_CHUNK_SIZE;
        current = current->next;
    }
    return capacity - STRUCT_ARENA_SIZE;
}

ARENA_API size_t Arena_GetCommitted(const Arena* self) {
    if (!self) return 0;
    return ARENA_IS_VIRTUAL(self->flags) ? self->committed : Arena_GetCapacity(self);
}

/**
 * INTERNAL HELPER FUNCTIONS
 */

static void Chunk_Destroy(Chunk* chunk, uint32_t flags) {
    if (!chunk) return;
    if (ARENA_IS_VIRTUAL(flags)) {
        VMem_Release(chunk->buffer, chunk->capacity);
    } else if (!ARENA_IS_CHILD(flags)) {
        free(chunk->buffer);
    }
}

static size_t Chunk_BaseOffset(const Arena* self, const Chunk* chunk) {
    return (chunk == self->head) ? (STRUCT_CHUNK_SIZE + STRUCT_ARENA_SIZE) : STRUCT_CHUNK_SIZE;
}

static bool Chunk_EnsureSpace(Arena* self, Chunk* chunk, size_t needed) {
    if (needed > chunk->capacity - chunk->offset) {
        lastError = "Arena is out of capacity.";
        return false;
    }

    if (ARENA_IS_VIRTUAL(self->flags)) {
        size_t required_commit = chunk->offset + needed;
        if (required_commit > self->committed) {
            size_t new_commit = GetNearestBlockSize(required_commit);
            if (new_commit == 0 || new_commit > chunk->capacity) {
                new_commit = chunk->capacity;
            }
            if (!VMem_Commit(chunk->buffer + self->committed, new_commit - self->committed)) {
                lastError = "Failed to commit virtual memory.";
                return false;
            }
            self->committed = new_commit;
        }
    }

    return true;
}

static Chunk* Chunk_Create(Arena* parent, uint32_t flags, size_t capacity, size_t* outCommitted) {
    if (capacity > SIZE_MAX - STRUCT_CHUNK_SIZE) {
        lastError = "Capacity is too large.";
        return NULL;
    }

    // For FIXED child arenas, carve the exact requested size (aligned) rather than rounding up to an OS page
    size_t total_size;
    if (ARENA_IS_CHILD(flags) && ARENA_IS_FIXED(flags)) {
        total_size = AlignUp(capacity + STRUCT_CHUNK_SIZE, alignof(max_align_t));
    } else {
        total_size = GetNearestBlockSize(capacity + STRUCT_CHUNK_SIZE);
    }
    // In case AlignUp overflows and wraps around to 0 (with very exact very large capacity)
    if (total_size == 0) {
        lastError = "Capacity is too large.";
        return NULL;
    }

    uint8_t* raw = NULL;
    size_t committed = 0;

    if (ARENA_IS_VIRTUAL(flags)) {
        raw = (uint8_t*)VMem_Reserve(total_size);
        if (!raw) {
            lastError = "Failed to reserve virtual memory.";
            return NULL;
        }

        // Commit just enough initial pages to hold the Chunk + Arena headers
        size_t initial_commit = GetNearestBlockSize(STRUCT_CHUNK_SIZE + STRUCT_ARENA_SIZE);
        if (initial_commit > total_size) initial_commit = total_size;

        if (!VMem_Commit(raw, initial_commit)) {
            VMem_Release(raw, total_size);
            lastError = "Failed to commit virtual memory.";
            return NULL;
        }
        committed = initial_commit;
    } else if (ARENA_IS_CHILD(flags)) {
        raw = (uint8_t*)Arena_Allocate(parent, total_size);
        if (!raw) return NULL; // Error is set by Arena_Allocate
    } else {
        raw = (uint8_t*)malloc(total_size);
        if (!raw) {
            lastError = "Failed to allocate memory from heap.";
            return NULL;
        }
    }

    Chunk* chunk = (Chunk*)raw;
    chunk->buffer = raw;
    chunk->capacity = total_size;
    chunk->offset = STRUCT_CHUNK_SIZE;
    chunk->next = NULL;

    if (outCommitted) *outCommitted = committed;
    return chunk;
}

static size_t GetSystemPageSize(void) {
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

static uint8_t GetSystemPageBitShift(void) {
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

static void* VMem_Reserve(size_t size) {
#if defined(_WIN32) || defined(_WIN64)
    return VirtualAlloc(NULL, size, MEM_RESERVE, PAGE_NOACCESS);
#else
    void* ptr = mmap(NULL, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    return (ptr == MAP_FAILED) ? NULL : ptr;
#endif
}

static int VMem_Commit(void* addr, size_t size) {
#if defined(_WIN32) || defined(_WIN64)
    return VirtualAlloc(addr, size, MEM_COMMIT, PAGE_READWRITE) != NULL;
#else
    return mprotect(addr, size, PROT_READ | PROT_WRITE) == 0;
#endif
}

static void VMem_Decommit(void* addr, size_t size) {
    if (size == 0) return;
#if defined(_WIN32) || defined(_WIN64)
    VirtualFree(addr, size, MEM_DECOMMIT);
#else
    madvise(addr, size, MADV_DONTNEED);
    mprotect(addr, size, PROT_NONE);
#endif
}

static void VMem_Release(void* addr, size_t reservedSize) {
#if defined(_WIN32) || defined(_WIN64)
    (void)reservedSize;
    VirtualFree(addr, 0, MEM_RELEASE);
#else
    munmap(addr, reservedSize);
#endif
}

static size_t CountFreeSlots(const Arena* self) {
    if (!ARENA_IS_POOLED(self->flags) || !ARENA_IS_BULK(self->flags)) {
        return 0;
    }
    size_t count = 0;
    void* current = self->freeList;
    while (current) {
        count++;
        current = *(void**)current;
    }
    return count;
}

static size_t CountStackFooters(const Arena* self) {
    if (!ARENA_IS_LINEAR(self->flags) || !ARENA_IS_STACK(self->flags)) {
        return 0;
    }
    size_t count = 0;
    const Chunk* chunk = self->head;
    while (chunk) {
        size_t base_offset = (chunk == self->head) ? (STRUCT_CHUNK_SIZE + STRUCT_ARENA_SIZE) : STRUCT_CHUNK_SIZE;
        size_t cursor = chunk->offset;
        while (cursor > base_offset) {
            stack_header_t* footer = (stack_header_t*)(chunk->buffer + cursor - STACK_HEADER_SIZE);
            cursor = *footer;
            count++;
        }
        chunk = chunk->next;
    }
    return count;
}

#ifndef NDEBUG
static bool Pool_IsValidActiveSlot(const Arena* self, const void* ptr) {
    const uint8_t* target = (const uint8_t*)ptr;
    const Chunk* chunk = self->head;
    bool in_bounds = false;

    // Verify ptr falls on a valid slot boundary within the allocated region of a chunk
    while (chunk) {
        size_t base_offset = Chunk_BaseOffset(self, chunk);
        const uint8_t* start = chunk->buffer + base_offset;
        const uint8_t* end   = chunk->buffer + chunk->offset;

        if (target >= start && target < end) {
            size_t rel_offset = (size_t)(target - start);
            if (rel_offset % self->slotSize == 0) {
                in_bounds = true;
            }
            break;
        }
        chunk = chunk->next;
    }

    if (!in_bounds) {
        lastError = "Pointer does not belong to a valid pool slot.";
        return false;
    }

    // Verify ptr is not already in the intrusive free list (double-free check)
    const void* node = self->freeList;
    while (node) {
        if (node == ptr) {
            lastError = "Double-free detected in pooled arena.";
            return false;
        }
        node = *(void* const*)node;
    }

    return true;
}
#endif