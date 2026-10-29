# The Unstandard C Library

My personal C libraries for my projects. Feel free to use them, modify them, update them or twist them to your own malicious purposes. If you include it in your own open source project without much change, you must include the license below.

No support is given, along with any fucks. But feel free to create issues and pull requests that do not break the API.

## C23

All libraries are made and tested with C23 as a baseline (but may miraculously work with earlier versions, or most likely not), so if you do not want any modern features in your code, you may need to find a different library made for C89. Or just compile it in C23 and link it to your app.

## Why not header-only?

Each folder contains its own thing, including a readme. Usually a `.h` and a `.c` file. I considered doing the stb header-only thing with `#define YOUR_LAZINESS_ON_DISPLAY_IMPLEMENTATION`, but I figured if you can't be arsed to copy TWO files rather than one, then what ARE you actually arsed about? Where you would use the define trick, you'll include the `.c` instead, and everywhere else you include the `.h`. Or just compile the `.c` and link it like a decent human being.

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