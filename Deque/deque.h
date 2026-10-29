#pragma once

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct DequeData {
    void *data;
    struct DequeData *next;
} DequeData;

typedef struct Deque {
    DequeData **data;
    size_t capacity;
    size_t size;
    size_t headIdx;
    size_t tailIdx;
} Deque;

Deque Deque_Create(size_t initialCapacity);

void Deque_Flush(Deque *deque, void (*flushFunction)(DequeData *));

void Deque_Free(Deque *deque, void (*flushFunction)(DequeData *));

size_t Deque_PushBack(Deque *deque, size_t args, ...);

size_t Deque_PushFront(Deque *deque, size_t args, ...);

size_t Deque_PopBack(Deque *deque, ...);

size_t Deque_PopFront(Deque *deque, ...);