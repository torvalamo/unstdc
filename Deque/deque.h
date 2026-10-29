#pragma once

#include <stdlib.h>
// #include <stdbool.h>

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
    // size_t args; // @todo Enforce argument count for all elements.
} Deque;

// struct DequeCreateOpts {
//     size_t initialCapacity;
//     bool preventResize; // @todo Option to prevent automatic resizing of the deque.
//     size_t elementArgs;
//     // Add more options here as needed.
// };

/** 
 * Creates a new deque with the specified initial capacity.
 *
 * @param initialCapacity The initial capacity of the deque.
 * @return The created deque.
 *
 * @warning If memory allocation fails, the Deque has capacity 0, and will grow by powers of 2 
 *          rather than by the `initialCapacity`.
 */
Deque Deque_Create(size_t initialCapacity);

// Deque Deque_Create_Opts(struct DequeCreateOpts opts);
// #define Deque_Create(...) Deque_Create_Opts((DequeCreateOpts){__VA_ARGS__})

/**
 * Flushes the deque, applying the specified flush function to each element.
 *
 * @param deque The deque to flush.
 * @param flushFunction The function to apply to each element.
 *
 * @warning YOU are responsible for freeing each chain of DequeData structs by following DequeData.next.
 *          When DequeData.next is NULL, you have reached the end of the chain for the current element.
 */
void Deque_Flush(Deque *deque, void (*flushFunction)(DequeData *));

/**
 * Frees the deque, applying the specified flush function to each element before freeing.
 *
 * @param deque The deque to free.
 * @param flushFunction The function to apply to each element before freeing.
 *
 * @warning YOU are responsible for freeing each chain of DequeData structs by following DequeData.next.
 *          When DequeData.next is NULL, you have reached the end of the chain for the current element.
 */
void Deque_Free(Deque *deque, void (*flushFunction)(DequeData *));

/**
 * Pushes an element with one or more arguments to the back of the deque.
 *
 * @param deque The deque to push the element to.
 * @param args The number of arguments for the element.
 * @param ... The arguments for the element, each as a void pointer.
 *
 * @return The new size of the deque after the push operation, or 0 if new memory allocation failed.
 */
size_t Deque_PushBack(Deque *deque, size_t args, ...);

/**
 * Pushes an element with one or more arguments to the front of the deque.
 *
 * @param deque The deque to push the element to.
 * @param args The number of arguments for the element.
 * @param ... The arguments for the element, each as a void pointer.
 *
 * @return The new size of the deque after the push operation, or 0 if new memory allocation failed.
 */
size_t Deque_PushFront(Deque *deque, size_t args, ...);

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
size_t Deque_PopBack(Deque *deque, ...);

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
size_t Deque_PopFront(Deque *deque, ...);