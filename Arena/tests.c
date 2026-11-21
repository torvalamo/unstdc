#include "arena.h"

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define RUN_TEST(fn) do { \
    printf("Running %-45s", #fn "..."); \
    fn(); \
    printf("PASSED\n"); \
} while (0)

typedef struct {
    int64_t a;
    int64_t b;
    double c;
} TestEntity;

// 1. Default: HEAP | DYNAMIC | LINEAR | BULK
static void Test_HeapDynamicLinearBulk(void) {
    Arena* arena = Arena_Initialize(0, 64);
    assert(arena != NULL);
    assert(Arena_Error() == NULL);
    assert(Arena_GetUserUsed(arena) == 0);

    size_t starting_cap = Arena_GetCapacity(arena);

    // Allocate enough to span multiple chunks regardless of OS page size (4KB / 16KB / 64KB)
    for (int i = 0; i < 5000; i++) {
        int* val = ARENA_ALLOC(arena, int);
        assert(val != NULL);
        *val = i;
    }

    size_t expanded_cap = Arena_GetCapacity(arena);
    assert(expanded_cap > starting_cap);
    assert(Arena_GetUserUsed(arena) > 0);
    assert(Arena_GetUsed(arena) == expanded_cap - Arena_GetUnused(arena));

    // Zero-initialized array & copy helpers
    int* arr = ARENA_ALLOC_ARRAY_Z(arena, 16, int);
    assert(arr != NULL);
    for (int i = 0; i < 16; i++) {
        assert(arr[i] == 0);
    }

    char* str = Arena_CopyString(arena, "Hello, Arena!");
    assert(str != NULL && strcmp(str, "Hello, Arena!") == 0);

    // Freeing on linear bulk should fail gracefully and set error
    assert(Arena_Free(arena, str) == false);
    assert(Arena_Error() != NULL);

    // Reset collapses extra chunks back to head
    Arena_Reset(arena);
    assert(Arena_GetUserUsed(arena) == 0);
    assert(Arena_GetCapacity(arena) == starting_cap);

    Arena_Destroy(arena);
}

// 2. Fixed Capacity: HEAP | FIXED | LINEAR | BULK
static void Test_HeapFixedLinearBulk(void) {
    Arena* arena = Arena_Initialize(ARENA_BACKING_HEAP | ARENA_GROWTH_FIXED, 128);
    assert(arena != NULL);

    size_t avail = Arena_GetAvailable(arena);
    assert(avail >= 128);

    // Exhaust capacity
    void* p1 = Arena_Allocate(arena, avail);
    assert(p1 != NULL);
    assert(Arena_GetAvailable(arena) == 0);

    // Next allocation must fail without growing
    void* p2 = Arena_Allocate(arena, 1);
    assert(p2 == NULL);
    assert(Arena_Error() != NULL);

    Arena_Destroy(arena);
}

// 3. Object Pool: HEAP | DYNAMIC | POOLED | BULK + Debug Guards
static void Test_PooledBulkAndDebugGuards(void) {
    Arena* pool = Arena_InitializeWithSlotSize(
        ARENA_LAYOUT_POOLED | ARENA_LIFETIME_BULK,
        sizeof(TestEntity) * 4,
        sizeof(TestEntity)
    );
    assert(pool != NULL);
    assert(Arena_GetUserUsed(pool) == 0);

    TestEntity* e1 = ARENA_ALLOC_Z(pool, TestEntity);
    TestEntity* e2 = ARENA_ALLOC(pool, TestEntity);
    TestEntity* e3 = ARENA_ALLOC(pool, TestEntity);
    assert(e1 && e2 && e3);
    assert(e1->a == 0 && e1->b == 0 && e1->c == 0.0);

    size_t used_three = Arena_GetUserUsed(pool);
    assert(used_three > 0);

    // Oversized allocation must fail
    void* too_big = Arena_Allocate(pool, 1024);
    assert(too_big == NULL);
    assert(Arena_Error() != NULL);

    // Free e2 out of order
    assert(Arena_Free(pool, e2) == true);
    assert(Arena_Error() == NULL);
    assert(Arena_GetUserUsed(pool) < used_three);

#ifndef NDEBUG
    // Double-free detection in debug builds
    assert(Arena_Free(pool, e2) == false);
    assert(Arena_Error() != NULL);

    // Unaligned / foreign pointer detection
    assert(Arena_Free(pool, (uint8_t*)e1 + 1) == false);
    assert(Arena_Error() != NULL);
#endif

    // Next allocation must reuse e2's exact slot
    TestEntity* e4 = ARENA_ALLOC(pool, TestEntity);
    assert(e4 == e2);
    assert(Arena_GetUserUsed(pool) == used_three);

    Arena_Destroy(pool);
}

// 4. Heterogeneous Linear Stack: HEAP | DYNAMIC | LINEAR | STACK
static void Test_LinearHeterogeneousStack(void) {
    Arena* stack = Arena_Initialize(ARENA_LAYOUT_LINEAR | ARENA_LIFETIME_STACK, 64);
    assert(stack != NULL);
    assert(Arena_GetUserUsed(stack) == 0);

    // Push mixed sizes across multiple chunks
    void* items[200];
    for (int i = 0; i < 200; i++) {
        size_t sz = (size_t)((i % 7 + 1) * 16);
        items[i] = Arena_Allocate(stack, sz);
        assert(items[i] != NULL);
        memset(items[i], (int)(i & 0xFF), sz);
    }
    assert(Arena_GetUserUsed(stack) > 0);

    // Out-of-order Arena_Free must fail LIFO check
    assert(Arena_Free(stack, items[0]) == false);
    assert(Arena_Error() != NULL);

    // Top-of-stack Arena_Free succeeds
    assert(Arena_Free(stack, items[199]) == true);
    assert(Arena_Error() == NULL);

    // Pop remaining 199 items back to completely empty
    for (int i = 198; i >= 0; i--) {
        assert(Arena_Pop(stack) == true);
        assert(Arena_Error() == NULL);
    }

    assert(Arena_GetUserUsed(stack) == 0);

    // Popping an empty stack fails and sets error
    assert(Arena_Pop(stack) == false);
    assert(Arena_Error() != NULL);

    Arena_Destroy(stack);
}

// 5. Homogeneous Vector Stack: HEAP | DYNAMIC | POOLED | STACK
static void Test_PooledHomogeneousVectorStack(void) {
    Arena* vec = Arena_InitializeWithSlotSize(
        ARENA_LAYOUT_POOLED | ARENA_LIFETIME_STACK,
        sizeof(TestEntity) * 2,
        sizeof(TestEntity)
    );
    assert(vec != NULL);

    TestEntity* pushed[100];
    for (int i = 0; i < 100; i++) {
        pushed[i] = ARENA_ALLOC(vec, TestEntity);
        assert(pushed[i] != NULL);
        pushed[i]->a = i;
    }

    // Wrong pointer fails LIFO check
    assert(Arena_Free(vec, pushed[0]) == false);
    assert(Arena_Error() != NULL);

    // Pop all 100 elements
    for (int i = 99; i >= 0; i--) {
        assert(pushed[i]->a == i);
        assert(Arena_Free(vec, pushed[i]) == true);
        assert(Arena_Error() == NULL);
    }

    assert(Arena_GetUserUsed(vec) == 0);
    Arena_Destroy(vec);
}

// 6. Virtual Memory Arena + Nested Child Sub-Arenas
static void Test_VirtualMasterAndNestedSubArenas(void) {
    // Reserve 64 MB of virtual address space
    size_t reserve_size = 64 * 1024 * 1024;
    Arena* vmem = Arena_Initialize(ARENA_BACKING_VIRTUAL, reserve_size);
    assert(vmem != NULL);

    size_t initial_committed = Arena_GetCommitted(vmem);
    assert(initial_committed > 0 && initial_committed < reserve_size);
    assert(Arena_GetCapacity(vmem) >= reserve_size);

    // Virtual arena cannot have a parent
    Arena* bad_child = Arena_InitializeWithParent(vmem, ARENA_BACKING_VIRTUAL, 4096);
    assert(bad_child == NULL);
    assert(Arena_Error() != NULL);

    // Carve a FIXED linear sub-arena inside the virtual master arena
    Arena* level_arena = Arena_InitializeWithParent(vmem, ARENA_GROWTH_FIXED, 16 * 1024);
    assert(level_arena != NULL);

    int* level_data = ARENA_ALLOC_ARRAY(level_arena, 1024, int);
    assert(level_data != NULL);
    level_data[0] = 42;
    level_data[1023] = 99;

    // Carve a DYNAMIC pooled sub-arena inside the virtual master arena
    Arena* entity_pool = Arena_InitializeWithParentAndSlotSize(
        vmem,
        ARENA_GROWTH_DYNAMIC | ARENA_LAYOUT_POOLED,
        sizeof(TestEntity) * 8,
        sizeof(TestEntity)
    );
    assert(entity_pool != NULL);

    for (int i = 0; i < 8; i++) {
        TestEntity* e = ARENA_ALLOC(entity_pool, TestEntity);
        assert(e != NULL);
        e->a = i;
    }

    // Committed virtual RAM should have grown to back the sub-arenas
    size_t peak_committed = Arena_GetCommitted(vmem);
    assert(peak_committed > initial_committed);

    // Destroying child arenas is a safe no-op (parent owns memory)
    Arena_Destroy(entity_pool);
    Arena_Destroy(level_arena);

    // Resetting virtual arena decommits physical pages back down to initial header page(s)
    Arena_Reset(vmem);
    assert(Arena_GetCommitted(vmem) == initial_committed);
    assert(Arena_GetUserUsed(vmem) == 0);

    Arena_Destroy(vmem);
}

int main(void) {
    RUN_TEST(Test_HeapDynamicLinearBulk);
    RUN_TEST(Test_HeapFixedLinearBulk);
    RUN_TEST(Test_PooledBulkAndDebugGuards);
    RUN_TEST(Test_LinearHeterogeneousStack);
    RUN_TEST(Test_PooledHomogeneousVectorStack);
    RUN_TEST(Test_VirtualMasterAndNestedSubArenas);

    printf("\nAll arena tests passed!\n");
    return 0;
}
    