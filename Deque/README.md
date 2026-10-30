# Deque - double ended queue

An adapted C implementation of std::deque from C++, with some extra flourish. It was created when following the Vulkan tutorials at vkguide.dev and translating to C on the fly, which is why it needs to practically adapt certain features that C++ does not have.

In this implementation you will pass 1 or more arguments to EACH entry in the deque, and every entry MUST have the same amount of arguments (unless you are very meticulous about the order of things). It is YOUR responsibility to know how many arguments the entries in a specific deque has.

You may only pass pointers as entries. Which means you will also be responsible for the memory of your things.

## API reference

### Deque Deque_Create_Opts(DequeCreateOpts opts)
### Deque Deque_Create([.initialCapacity = 0], [.elementArgs = 0])

A macro for `Deque_Create_Opts` that allows this style of declarative options.

Returns a new `Deque` set up with any given set of opts. You never change anything on the `Deque` struct manually (in that case you probably invalidate something and your program will crash). 

The deque will double its capacity each time it is full.

**Opts**:
* `.initialCapacity = (int)` - Initial memory capacity. Default 0.
* `.elementArgs = (int)` - Ensure each element has the same number of arguments. Default 0 = variatic arguments allowed.

Because the deque only accepts pointers, we do not allow you to clear it without handling the contents with the following functions.

### void Deque_Flush(Deque *deque, void (*flushFunction)(DequeData *))

> **Heads up**: You probably want to just run the pop functions below in a loop rather than use this one for "clearing out".

Run through all the items and pass it to the `flushFunction`, then clear the Deque (keeping its current size).

It is your `flushFunction`'s job to free each DequeData and it's chain.

```c
struct DequeData {
    void *data;
    struct DequeData *next;
};
```

`next` is NULL when you are at the end of the chain.

```c
void my_flush_func(DequeData *dd) {
    size_t index = 0;
    DequeData *cur = dd;
    DequeData *prev = NULL;
    while (cur != NULL && (
        // ...
        // do something with your cur->data, 
        // possibly based on the index
        // ...
        index++,
        prev = cur,
        cur = cur->next,
        free(prev),
        1));
}

Deque_Flush(&deque, my_flush_func);
```

### void Deque_Free(Deque *deque, void (*flushFunction)(DequeData *))

Does the same as `Deque_Flush`, except it also frees all data memory for the Deque itself, leaving it at size 0. It will then expand dynamically by powers of two as it grows.

### size_t Deque_PushBack(Deque *deque, ...) / size_t Deque_PushFront(Deque *deque, ...)

Insert an element with the expected number of elements at the back or front of the deque. The expected number of elements is the given `.elementArgs` in struct creation options. If it was not set, this function does nothing and returns 0.

### size_t Deque_PushBackEx(Deque *deque, size_t args, ...) / size_t Deque_PushFrontEx(Deque *deque, size_t args, ...)

Insert an element with `args` number of arguments at the back or front of the deque. If `args` is 0 or if `.elementArgs` is set and is not identical to `args`, this function does nothing and will return 0.

```c
Deque_PushBackEx(&deque, 1, &something);
Deque_PushFrontEx(&double_deque, 2, &some_func, &some_arg);
```

You may insert an arbitrary number of arguments for each element, but it is YOUR responsibility to know which element you extract and exactly how many arguments it holds. Therefore it is usually safest to keep each `Deque` uniform in its argument count, if not also its types.

You could also do something like:

```c
Deque_PushBackEx(&deque, 4, (void *)(uintptr_t)ELEMENT_TYPE_A, /* 3 more args */);
//...
void *raw_type;
Deque_PopBack(&deque, &raw_type, /* 3 more args */);
TypeEnum type = (TypeEnum)(uintptr_t)raw_type;
switch(type) {
    case ELEMENT_TYPE_A:
    //...
}
```

Which would use the first argument as an identifier (be careful to NEVER use it as an actual pointer).

### size_t Deque_PopBack(Deque *deque, ...) / size_t Deque_PopFront(Deque *deque, ...)

Remove an element from the back or front of the deque, inserting its arguments into the pointers given as function arguments.

It is YOUR responsibility to ensure the correct number of arguments are given. The function will keep reading function arguments until the element's arguments are exhausted, regardless of the number of arguments given to the function. If you supply too few, your program will likely crash.

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