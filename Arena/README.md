# arena - memory arena

C implementation of memory arenas with varied features.

Backing: Can allocate on heap (default), in virtual memory, or using a parent arena (parent must be virtual).
Growth: Can be dynamic (default) or finite size.
Layout: Can provide heterogenous/linear allocation (default) or pooled/fixed-size allocation.
Lifetime: Can free in bulk (default), reuse slots (only pooled arenas) or push/pop on a LIFO stack

## API reference

[arena.h](arena.h) documents everything for now.

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