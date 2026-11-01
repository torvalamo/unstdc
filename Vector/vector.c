#include "vector.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

Vector Vector_Create_Opts(size_t typeSize, VectorCreateOpts opts) {
    size_t initialCapacity = opts.initialCapacity;
    void *data = initialCapacity ? malloc(initialCapacity * typeSize) : NULL;
    Vector vector = {
        .capacity = data ? initialCapacity : 0,
        .typeSize = typeSize,
        .size = 0,
        .data = data
    };
    return vector;
}

void Vector_Free(Vector *vector) {
    free(vector->data);
    vector->data = NULL;
    vector->size = 0;
    vector->capacity = 0;
}

static bool Vector_EnsureCapacity(Vector *vector) {
    if (vector->size == vector->capacity) {
        size_t newCapacity = vector->capacity ? vector->capacity * 2 : 1;
        void *newData = realloc(vector->data, newCapacity * vector->typeSize);
        if (!newData) return false;
        vector->data = newData;
        vector->capacity = newCapacity;
    }
    return true;
}

size_t Vector_PushBack(Vector *vector, const void *data) {
    if (!Vector_EnsureCapacity(vector)) return vector->size;
    memcpy((char *)vector->data + vector->size * vector->typeSize, data, vector->typeSize);
    return vector->size++;
}

size_t Vector_PopBack(Vector *vector) {
    if (vector->size == 0) return 0;
    return --vector->size;
}

bool Vector_Insert(Vector *vector, size_t index, const void *data) {
    if (index > vector->size) return false;
    if (!Vector_EnsureCapacity(vector)) return false;
    memmove((char *)vector->data + (index + 1) * vector->typeSize,
            (char *)vector->data + index * vector->typeSize,
            (vector->size - index) * vector->typeSize);
    memcpy((char *)vector->data + index * vector->typeSize, data, vector->typeSize);
    vector->size++;
    return true;
}

bool Vector_Erase(Vector *vector, size_t index) {
    if (index >= vector->size) return false;
    memmove((char *)vector->data + index * vector->typeSize,
            (char *)vector->data + (index + 1) * vector->typeSize,
            (vector->size - index - 1) * vector->typeSize);
    vector->size--;
    return true;
}

void Vector_Clear(Vector *vector) {
    vector->size = 0;
}

size_t Vector_Assign(Vector *vector, size_t count, const void *data) {
    if (count > vector->capacity) {
        size_t newCapacity = count;
        void *newData = realloc(vector->data, newCapacity * vector->typeSize);
        if (!newData) return vector->size;
        vector->data = newData;
        vector->capacity = newCapacity;
    }
    if (count > 0 && data != NULL) {
        memcpy(vector->data, data, count * vector->typeSize);
    }
    vector->size = count;
    return vector->size;
}

const void *Vector_At(Vector *vector, size_t index) {
    if (index >= vector->size) return NULL;
    return (char *)vector->data + index * vector->typeSize;
}

void *Vector_AtMut(Vector *vector, size_t index) {
    if (index >= vector->size) return NULL;
    return (char *)vector->data + index * vector->typeSize;
}

const void *Vector_Back(Vector *vector) {
    if (vector->size == 0) return NULL;
    return (char *)vector->data + (vector->size - 1) * vector->typeSize;
}

const void *Vector_Front(Vector *vector) {
    if (vector->size == 0) return NULL;
    return vector->data;
}

const void *Vector_Data(Vector *vector) {
    return vector->data;
}

size_t Vector_TypeSize(Vector *vector) {
    return vector->typeSize;
}

size_t Vector_Size(Vector *vector) {
    return vector->size;
}

size_t Vector_Capacity(Vector *vector) {
    return vector->capacity;
}

bool Vector_Empty(Vector *vector) {
    return vector->size == 0;
}

size_t Vector_Reserve(Vector *vector, size_t newCapacity) {
    if (newCapacity <= vector->capacity) return vector->capacity;
    void *newData = realloc(vector->data, newCapacity * vector->typeSize);
    if (!newData) return vector->capacity;
    vector->data = newData;
    vector->capacity = newCapacity;
    return vector->capacity;
}

size_t Vector_Resize(Vector *vector, size_t newSize, const void *defaultData) {
    if (newSize > vector->capacity) {
        size_t newCapacity = newSize;
        void *newData = realloc(vector->data, newCapacity * vector->typeSize);
        if (!newData) return vector->size;
        vector->data = newData;
        vector->capacity = newCapacity;
    }
    if (newSize > vector->size) {
        if (defaultData != NULL) {
            for (size_t i = vector->size; i < newSize; i++) {
                memcpy((char *)vector->data + i * vector->typeSize, defaultData, vector->typeSize);
            }
        } else {
            memset((char *)vector->data + vector->size * vector->typeSize, 0, (newSize - vector->size) * vector->typeSize);
        }
    }
    vector->size = newSize;
    return vector->size;
}

size_t Vector_ShrinkToFit(Vector *vector) {
    if (vector->size == vector->capacity) return vector->capacity;
    if (vector->size == 0) {
        free(vector->data);
        vector->data = NULL;
        vector->capacity = 0;
        return 0;
    }
    void *newData = realloc(vector->data, vector->size * vector->typeSize);
    if (!newData) return vector->capacity;
    vector->data = newData;
    vector->capacity = vector->size;
    return vector->capacity;
}

void Vector_Swap(Vector *vector1, Vector *vector2) {
    Vector temp = *vector1;
    *vector1 = *vector2;
    *vector2 = temp;
}