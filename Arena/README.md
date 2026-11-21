# arena - memory arena

C implementation of memory arenas with varied features.

There are also a few metrics functions. See the header file for a list.

> **TIP:** When a function returns `NULL` or `false` that normally shouldn't, you can check the error message with `char const* Arena_Error()`.

## Flags reference

You can combine most of these flags (one per type, obviously).

### ARENA_BACKING_HEAP (default)

Allocates memory blocks on the C heap (`malloc`) rounded up to the nearest OS page multiple (e.g., 4 KB on x86_64, 16 KB on Apple Silicon).

* **Arena_Initialize()**: The arena stores its own control headers at the beginning of this initial chunk.

* **Arena_Allocate()**: Serves allocations from the active tail chunk. If the tail chunk does not have enough remaining space, then either allocate a new chunk (default) or return NULL and an error (if ARENA_GROWTH_FIXED).

* **Arena_Reset()**: Frees any excess dynamic chunks and returns to the initial state of the first chunk.

* **Arena_Destroy()**: Frees everything related to the arena, including the arena itself.

### ARENA_BACKING_VIRTUAL

Sets aside a contiguous range of virtual addresses rounded up to the OS page size (e.g., 4 KB on x86_64, 16 KB on Apple Silicon).

* **Arena_Initialize()**: It is normal to reserve a massive range that your app might use, e.g. several GB if necessary. The arena itself has some overhead, so the minimum actual use is 1 OS page size, then it commits physical pages within that reserved range as needed.

> **Note for 32-bit systems:** The maximum virtual memory for 32-bit systems is about 2 GB. For 64-bit systems it is in the terabytes.

* **Arena_Allocate()**: Commits more of the virtual memory range to physical RAM pages, as the need grows.

* **Arena_Free() and Arena_Pop()**: In a virtual arena, popping and freeing pooled or stacked resources, resulting in "empty" tailing pages, will not decommit them. The only way to actually free physical memory in a virtual arena is to reset or destroy.

* **Arena_Reset()**: Keeps the reserved virtual memory range intact, resets the offset to its initial state, and decommits excess physical pages back to the OS.

* **Arena_Destroy()**: Unmaps the entire virtual address range and frees all remaining physical pages.

### Parent Arena Backing (`parent != NULL`)

Passing a `parent` arena to `Arena_InitializeWithParent*` carves the child arena's memory directly out of the parent instead of calling `malloc` or reserving OS virtual memory. Cannot be combined with `ARENA_BACKING_VIRTUAL`.

* **Arena_Reset()**: Resets the child back to its initial chunk while keeping any already-carved overflow chunks attached for zero-overhead reuse on future allocations, avoiding repeated allocations from `parent`.

### ARENA_GROWTH_DYNAMIC (default)

When reaching capacity, a dynamic arena allocates additional chunks and continues from there.

### ARENA_GROWTH_FIXED

When reaching capacity, a fixed arena fails and returns `NULL` with an error message retrievable via `Arena_Error()`.

> **Note on GROWTH in Virtual Arenas:** In an `ARENA_BACKING_VIRTUAL` arena, both behaviors effectively work together regardless of which growth flag is passed — the initial `capacity` acts as a hard ceiling on reserved virtual address space, while physical RAM is committed dynamically in pages as needed.

### ARENA_LAYOUT_LINEAR (default)

Allocations can be of any size and are placed sequentially.

* **Best for:** Heterogeneous data such as mixed structs, strings, dynamic arrays, and AST/tree nodes.
* **Individual freeing:** Not supported with `ARENA_LIFETIME_BULK`; supported in strict LIFO order when combined with `ARENA_LIFETIME_STACK`.

### ARENA_LAYOUT_POOLED

Divides the arena into uniform slots of `slotSize` (rounded up to `alignof(max_align_t)`, usually 16 on 64-bit), specified via `Arena_InitializeWithSlotSize()` or `Arena_InitializeWithParentAndSlotSize()`.

* **Allocation size:** Passing `0` or any size `<= slotSize` returns a slot; requesting a size larger than `slotSize` fails and returns `NULL`. Array allocations (`Arena_AllocateArray*`) are not supported.
* **With `ARENA_LIFETIME_BULK` (default):** Calling `Arena_Free()` adds the slot to an internal intrusive free list, allowing future allocations to reuse freed slots out of order in O(1) time.
* **With `ARENA_LIFETIME_STACK`:** Operates as a contiguous vector-style stack where `Arena_Pop()` and `Arena_Free()` roll back the top slot without needing a free list or per-allocation metadata.

### ARENA_LIFETIME_BULK (default)

Memory is reclaimed all at once via `Arena_Reset()` or `Arena_Destroy()`.

* **With `ARENA_LAYOUT_POOLED`:** Calling `Arena_Free()` recycles individual slots out of order via an internal free list.

### ARENA_LIFETIME_STACK

Reclaims memory in strict LIFO (Last-In, First-Out) order without needing external checkpoint tokens.

* **`Arena_Pop()`:** Rolls back the most recently allocated block (or slot, if combined with `ARENA_LAYOUT_POOLED`). On dynamic heap arenas, completely emptied overflow chunks are freed immediately.
* **`Arena_Free()`:** Validates that `ptr` is the current top item of the stack before popping it; passing an out-of-order pointer returns `false` and sets an error.

## Code examples

```c
Arena* virtualArena = Arena_Initialize(ARENA_BACKING_VIRTUAL, 1024ULL*1024*1024); // Reserve 1 GB
MyData* data = Arena_Allocate(virtualArena, sizeof(MyData));
Arena* childArena = Arena_InitializeWithParentAndSlotSize(virtualArena, ARENA_GROWTH_FIXED | ARENA_LAYOUT_POOLED, 64, sizeof(float)); // sizeof(float) is 4, but it's aligned to 16
float* a = Arena_Allocate(childArena, sizeof(float));
float* b = ARENA_ALLOC(childArena, float); // Shorthand macro, same as the line above
Arena_GetUserUsed(childArena); // 32 bytes used (2 floats of 16 aligned)
Arena_Free(childArena, a); a = NULL;
float* c = Arena_Allocate(childArena, sizeof(float)); // reuses the available slot from a.
Arena_GetUserUsed(childArena); // should return 32 bytes used
Arena_GetUserUsed(virtualArena); // should return [aligned sizeof(MyData)] + 64 + [aligned childArena overhead]

Arena_Destroy(virtualArena); // Also destroys the childArena (which is fine if this is the end of the program)
```

```c
// Heterogeneous LIFO stack (supports mixed allocation sizes)
Arena* scratch = Arena_Initialize(ARENA_LAYOUT_LINEAR | ARENA_LIFETIME_STACK, 4096);

int* number = ARENA_ALLOC(scratch, int);
char* text  = Arena_CopyString(scratch, "Temporary string");
float* vec  = ARENA_ALLOC_ARRAY(scratch, float, 4);

Arena_Free(scratch, number); // Returns false (not at top of stack; sets Arena_Error())
Arena_Free(scratch, vec);    // Succeeds and pops 'vec'
Arena_Pop(scratch);          // Pops 'text'
Arena_Pop(scratch);          // Pops 'number' — Arena_GetUserUsed(scratch) is now 0

Arena_Destroy(scratch);
```

## API reference

[arena.h](arena.h) documents everything for now.

## Tests

```bash
clang -std=c11 -Wall -Wextra -g -fsanitize=address,undefined tests.c arena.c -o tests && ./tests
```

(replace `clang` with your compiler)

## TODO list

Nothing yet.

## MIT License

Copyright (c) 2026 Tor Valamo

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.