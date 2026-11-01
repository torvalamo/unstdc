#pragma once

#include <stddef.h>
#include <stdbool.h>

typedef struct Vector {
    void *data;
    size_t typeSize;
    size_t capacity;
    size_t size;
} Vector;

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