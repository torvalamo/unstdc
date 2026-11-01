#pragma once

#include <stddef.h>

typedef struct Span {
    const void *data;
    size_t size;
    size_t typeSize;
} Span;

Span Span_Create(const void *data, size_t size, size_t typeSize);
const void *Span_Front(Span *span);
const void *Span_Back(Span *span);
const void *Span_At(Span *span, size_t index);
const void *Span_Data(Span *span);

Span Span_Subspan(Span *span, size_t offset, size_t size);

size_t Span_Size(Span *span);
size_t Span_TypeSize(Span *span);
size_t Span_SizeBytes(Span *span);