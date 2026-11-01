#include "span.h"

Span Span_Create(const void *data, size_t size, size_t typeSize) {
    Span span = { 
        .data = data, 
        .size = size, 
        .typeSize = typeSize 
    };
    return span;
}

const void *Span_Front(Span *span) {
    return span->data;
}

const void *Span_Back(Span *span) {
    return (const char *)span->data + (span->size - 1) * span->typeSize;
}

const void *Span_At(Span *span, size_t index) {
    return (const char *)span->data + index * span->typeSize;
}

const void *Span_Data(Span *span) {
    return span->data;
}

Span Span_Subspan(Span *span, size_t offset, size_t size) {
    return Span_Create((const char *)span->data + offset * span->typeSize, size, span->typeSize);
}

size_t Span_Size(Span *span) {
    return span->size;
}

size_t Span_TypeSize(Span *span) {
    return span->typeSize;
}

size_t Span_SizeBytes(Span *span) {
    return span->size * span->typeSize;
}