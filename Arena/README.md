# arena - memory arena

C implementation of an automatically expanding memory arena.

## API reference

### Arena Arena_Initialize()

Initializes and returns a new arena.

### void Arena_Destroy(Arena *self)

Destroys the arena and frees all associated memory.

### void *Arena_Allocate(Arena *self, size_t size)

Allocates memory from the arena. 

Returns a pointer to the allocated memory, or NULL on failure.

### void Arena_Reset(Arena *self)

Resets the arena to its original size and frees any expanded chunks.

**Warning**: Ensure all requested memory is no longer in use before calling this function.

### char *Arena_CopyString(Arena *self, const char *str)

Copies a string into the arena. 

Returns a pointer to the copied string, or NULL on failure.

## Macros

### ARENA_ALLOC(arena, type)

Allocates memory for a single instance of a type from the arena. 

Returns a pointer to the allocated memory, or NULL on failure.

### ARENA_ALLOC_ARRAY(arena, type, count)

Allocates memory for an array of instances of a type from the arena.

Returns a pointer to the allocated memory, or NULL on failure.

### ARENA_ALLOC_STRING(arena, str)

Allocates memory for a string from the arena.

Returns a pointer to the allocated memory, or NULL on failure.

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