# vector - dynamic contiguous array

A C implementation of something std::vector like.

Not to be confused with math/graphics vectors (vec2, vec3 etc.).

## API reference

### Vector Vector_Create_Opts(size_t typeSize, VectorCreateOpts opts)
### Vector Vector_Create(size_t typeSize, [.initialCapacity = 0])

### size_t Vector_PushBack(Vector *vector, void *data)

It memcpy's `typeSize` amount of data starting at `data`.

```c
typedef struct VectorCreateOpts {
    size_t initialCapacity;
} VectorCreateOpts;

Vector Vector_Create_Opts(size_t typeSize, VectorCreateOpts opts);
#define Vector_Create(typeSize, ...) Vector_Create_Opts(typeSize, (VectorCreateOpts){__VA_ARGS__})
void Vector_Free(Vector *vector);

size_t Vector_PushBack(Vector *vector, const void *data);
size_t Vector_PopBack(Vector *vector);
bool Vector_Insert(Vector *vector, size_t index, const void *data);
bool Vector_Erase(Vector *vector, size_t index);
void Vector_Clear(Vector *vector);
size_t Vector_Assign(Vector *vector, size_t count, const void *data);

const void *Vector_At(Vector *vector, size_t index);
void *Vector_AtMut(Vector *vector, size_t index);
const void *Vector_Back(Vector *vector);
const void *Vector_Front(Vector *vector);
const void *Vector_Data(Vector *vector);

size_t Vector_TypeSize(Vector *vector);
size_t Vector_Size(Vector *vector);
size_t Vector_Capacity(Vector *vector);
bool Vector_Empty(Vector *vector);
size_t Vector_Reserve(Vector *vector, size_t newCapacity);
size_t Vector_Resize(Vector *vector, size_t newSize, const void *defaultData);
size_t Vector_ShrinkToFit(Vector *vector);

void Vector_Swap(Vector *vector1, Vector *vector2);
```

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