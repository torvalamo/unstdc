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
            deque->data = oldData;
            deque->capacity = oldCapacity;
            return false;
        }
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

Deque Deque_Create_Opts(struct DequeCreateOpts opts) {
    size_t initialCapacity = opts.initialCapacity;
    size_t elementArgs = opts.elementArgs;
    DequeData **data = initialCapacity ? malloc(initialCapacity * sizeof(DequeData *)) : NULL;
    Deque deque = {
        .capacity = data ? initialCapacity : 0,
        .size = 0,
        .headIdx = 0,
        .tailIdx = 0,
        .data = data,
        .args = elementArgs
    };
    return deque;
}

void Deque_Flush(Deque *deque, void (*flushFunction)(DequeData *)) {
    if (flushFunction) {
        for (size_t i = 0; i < deque->size; i++) {
            size_t idx = (deque->headIdx + i) % deque->capacity;
            flushFunction(deque->data[idx]);
        }
    }
    Deque_Clear(deque);
}

void Deque_Free(Deque *deque, void (*flushFunction)(DequeData *)) {
    Deque_Flush(deque, flushFunction);
    free(deque->data);
    deque->data = NULL;
    deque->capacity = 0;
}

// Macros for initializing and handling push operations in the deque.
// The push operation code essentially repeats four times with minor variations.
#define DEQUE_PUSH_INIT_CODE(increaseDirection, preArgument) \
    va_list argsList; \
    va_start(argsList, preArgument); \
    if (!Deque_EnsureCapacity(deque)) { \
        return 0; \
    } \
    if (deque->size > 0) { \
        increaseDirection % deque->capacity; \
    } else { \
        deque->headIdx = 0; \
        deque->tailIdx = 0; \
    }
// Shorthands so that the code snippet is consistent.
#define DEQUE_PUSHBACK_INIT_CODE(preArgument) DEQUE_PUSH_INIT_CODE(deque->tailIdx = (deque->tailIdx + 1), preArgument)
#define DEQUE_PUSHFRONT_INIT_CODE(preArgument) DEQUE_PUSH_INIT_CODE(deque->headIdx = (deque->headIdx + deque->capacity - 1), preArgument)
#define DEQUE_PUSH_LOOP_CODE(direction, args) \
    DequeData *prev = NULL; \
    for (size_t i = 0; i < args; i++) { \
        DequeData *dd = (DequeData *)malloc(sizeof(DequeData)); \
        if (!dd) { \
            va_end(argsList); \
            return 0; \
        } \
        dd->data = va_arg(argsList, void *); \
        if (i == 0) { \
            deque->data[direction] = dd; \
        } \
        if (prev) { \
            prev->next = dd; \
        } \
        prev = dd; \
    } \
    va_end(argsList); \
    deque->size++; \
    return deque->size;

size_t Deque_PushBack(Deque *deque, ...) {
    if (deque->args == 0) return 0;
    DEQUE_PUSHBACK_INIT_CODE(deque);
    DEQUE_PUSH_LOOP_CODE(deque->tailIdx, deque->args);
}

size_t Deque_PushFront(Deque *deque, ...) {
    if (deque->args == 0) return 0;
    DEQUE_PUSHFRONT_INIT_CODE(deque);
    DEQUE_PUSH_LOOP_CODE(deque->headIdx, deque->args);
}

size_t Deque_PushBackEx(Deque *deque, size_t args, ...) {
    if (args == 0 || (deque->args == 0 && deque->args != args)) return 0;
    DEQUE_PUSHBACK_INIT_CODE(args);
    DEQUE_PUSH_LOOP_CODE(deque->tailIdx, args);
}

size_t Deque_PushFrontEx(Deque *deque, size_t args, ...) {
    if (args == 0 || (deque->args == 0 && deque->args != args)) return 0;
    DEQUE_PUSHFRONT_INIT_CODE(args);
    DEQUE_PUSH_LOOP_CODE(deque->headIdx, args);
}

// Undefine the push operation macros to avoid polluting the global namespace.
#undef DEQUE_PUSH_INIT_CODE
#undef DEQUE_PUSHBACK_INIT_CODE
#undef DEQUE_PUSHFRONT_INIT_CODE
#undef DEQUE_PUSH_LOOP_CODE

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