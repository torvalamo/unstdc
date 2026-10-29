#include "deque.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

static void Deque_Clear(Deque *deque) {
    deque->size = 0;
    deque->headIdx = 0;
    deque->tailIdx = 0;
}

static bool Deque_EnsureCapacity(Deque *deque) {
    if (deque->size == deque->capacity) {
        const size_t oldCapacity = deque->capacity;
        deque->capacity = deque->capacity ? deque->capacity * 2 : 1;
        DequeData **oldData = deque->data;
        deque->data = realloc(deque->data, deque->capacity * sizeof(DequeData *));
        if (!deque->data) {
            deque->data = oldData; // Restore old data pointer on failure
            deque->capacity = oldCapacity; // Restore old capacity on failure
            return false;
        }
        // If head is later than tail,
        // move elements between 0 to tail to the start of the new half of the array
        if (deque->headIdx > deque->tailIdx) {
            memmove(deque->data + oldCapacity, deque->data, (deque->tailIdx + 1) * sizeof(DequeData *));
            deque->tailIdx += oldCapacity;
        }
    }
    return true;
}

static size_t Deque_DataDereference(DequeData *dd, va_list *argsList) {
    size_t count = 0;
    DequeData *current = dd;
    DequeData *prev = NULL;
    while (current != NULL && (
        count++,
        *(va_arg(*argsList, void **)) = current->data,
        prev = current,
        current = current->next,
        free(prev),
        1));
    return count;
}

/** 
 * Creates a new deque with the specified initial capacity.
 *
 * @param initialCapacity The initial capacity of the deque.
 * @return The created deque.
 */
Deque Deque_Create(size_t initialCapacity) {
    Deque deque = {
        .capacity = initialCapacity,
        .size = 0,
        .headIdx = 0,
        .tailIdx = 0,
        .data = initialCapacity ? malloc(initialCapacity * sizeof(DequeData *)) : NULL
    };
    return deque;
}

/**
 * Flushes the deque, applying the specified flush function to each element.
 *
 * @param deque The deque to flush.
 * @param flushFunction The function to apply to each element.
 *
 * @warning YOU are responsible for freeing each chain of DequeData structs by following DequeData.next.
 *          When DequeData.next is NULL, you have reached the end of the chain for the current element.
 */
void Deque_Flush(Deque *deque, void (*flushFunction)(DequeData *)) {
    if (flushFunction) {
        for (size_t i = 0; i < deque->size; i++) {
            size_t idx = (deque->headIdx + i) % deque->capacity;
            flushFunction(deque->data[idx]);
        }
    }
    Deque_Clear(deque);
}
/**
 * Frees the deque, applying the specified flush function to each element before freeing.
 *
 * @param deque The deque to free.
 * @param flushFunction The function to apply to each element before freeing.
 *
 * @warning YOU are responsible for freeing each chain of DequeData structs by following DequeData.next.
 *          When DequeData.next is NULL, you have reached the end of the chain for the current element.
 */
void Deque_Free(Deque *deque, void (*flushFunction)(DequeData *)) {
    Deque_Flush(deque, flushFunction);
    free(deque->data);
    deque->data = NULL;
    deque->capacity = 0;
    // The deque is fully cleared and has capacity set to 0. 
    // It can safely be reused by pushing or unshifting new elements to it, 
    // in which case it will scale automatically by powers of 2, not its original initial capacity.
}

/**
 * Pushes an element with one or more arguments to the back of the deque.
 *
 * @param deque The deque to push the element to.
 * @param args The number of arguments for the element.
 * @param ... The arguments for the element, each as a void pointer.
 *
 * @return The new size of the deque after the push operation, or 0 if new memory allocation failed.
 */
size_t Deque_PushBack(Deque *deque, size_t args, ...) {
    if (!Deque_EnsureCapacity(deque)) {
        return 0;
    }
    if (deque->size > 0) {
        deque->tailIdx = (deque->tailIdx + 1) % deque->capacity;
    } else {
        deque->headIdx = 0;
        deque->tailIdx = 0;
    }
    va_list argsList;
    va_start(argsList, args);
    DequeData *prev = NULL;
    for (size_t i = 0; i < args; i++) {
        DequeData *dd = (DequeData *)malloc(sizeof(DequeData));
        if (!dd) {
            va_end(argsList);
            return 0;
        }
        dd->data = va_arg(argsList, void *);
        if (i == 0) {
            deque->data[deque->tailIdx] = dd;
        }
        if (prev) {
            prev->next = dd;
        }
        prev = dd;
    }
    va_end(argsList);
    deque->size++;
    return deque->size;
}

/**
 * Pushes an element with one or more arguments to the front of the deque.
 *
 * @param deque The deque to push the element to.
 * @param args The number of arguments for the element.
 * @param ... The arguments for the element, each as a void pointer.
 *
 * @return The new size of the deque after the push operation, or 0 if new memory allocation failed.
 */
size_t Deque_PushFront(Deque *deque, size_t args, ...) {
    if (!Deque_EnsureCapacity(deque)) {
        return 0;
    }
    if (deque->size > 0) {
        deque->headIdx = (deque->headIdx + deque->capacity - 1) % deque->capacity;
    } else {
        deque->headIdx = 0;
        deque->tailIdx = 0;
    }
    va_list argsList;
    va_start(argsList, args);
    DequeData *prev = NULL;
    for (size_t i = 0; i < args; i++) {
        DequeData *dd = (DequeData *)malloc(sizeof(DequeData));
        if (!dd) {
            va_end(argsList);
            return 0;
        }
        dd->data = va_arg(argsList, void *);
        if (i == 0) {
            deque->data[deque->headIdx] = dd;
        }
        if (prev) {
            prev->next = dd;
        }
        prev = dd;
    }
    va_end(argsList);
    deque->size++;
    return deque->size;
}

/**
 * Pops an element from the back of the deque, retrieving its arguments.
 *
 * @param deque The deque to pop the element from.
 * @param ... Pointers to variables where the arguments of the element will be stored.
 *
 * @return The number of arguments retrieved, or 0 if the deque was empty.
 *
 * @note The arguments must be provided in the same order as they were pushed.
 */
size_t Deque_PopBack(Deque *deque, ...) {
    if (deque->size == 0) {
        Deque_Clear(deque);
        return 0;
    }
    va_list argsList;
    va_start(argsList, deque);
    size_t count = Deque_DataDereference(deque->data[deque->tailIdx], &argsList);
    va_end(argsList);
    deque->data[deque->tailIdx] = NULL;
    if (deque->size > 1) {
        deque->tailIdx = (deque->tailIdx + deque->capacity - 1) % deque->capacity;
        deque->size--;
    } else {
        Deque_Clear(deque);
    }
    return count;
}

/**
 * Pops an element from the front of the deque, retrieving its arguments.
 *
 * @param deque The deque to pop the element from.
 * @param ... Pointers to variables where the arguments of the element will be stored.
 *
 * @return The number of arguments retrieved, or 0 if the deque was empty.
 *
 * @note The arguments must be provided in the same order as they were pushed.
 */
size_t Deque_PopFront(Deque *deque, ...) {
    if (deque->size == 0) {
        Deque_Clear(deque);
        return 0;
    }
    va_list argsList;
    va_start(argsList, deque);
    size_t count = Deque_DataDereference(deque->data[deque->headIdx], &argsList);
    va_end(argsList);
    deque->data[deque->headIdx] = NULL;
    if (deque->size > 1) {
        deque->headIdx = (deque->headIdx + 1) % deque->capacity;
        deque->size--;
    } else {
        Deque_Clear(deque);
    }
    return count;
}