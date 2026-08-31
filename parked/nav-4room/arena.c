#include "aw.h"

void ArenaInit(Arena *a, u8 *base, size_t size) {
    a->base = base; a->size = size; a->used = 0;
}

void *ArenaPush(Arena *a, size_t size, size_t align) {
    size_t p = (size_t)(a->base + a->used);
    size_t pad = (align - (p & (align - 1))) & (align - 1);
    if (a->used + pad + size > a->size) return 0;   // fixed budget; callers assert
    void *r = a->base + a->used + pad;
    a->used += pad + size;
    return r;
}

void ArenaReset(Arena *a) { a->used = 0; }
