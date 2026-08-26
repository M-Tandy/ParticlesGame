#include <stdlib.h>

#include "hash.h"

int Hash_6432shift(long key) {
    // From: https://gist.github.com/badboy/6267743
    key = (~key) + (key << 18); // key = (key << 18) - key - 1;
    key = key ^ (key >> 31);
    key = key * 21; // key = (key + (key << 2)) + (key << 4);
    key = key ^ (key >> 11);
    key = key + (key << 6);
    key = key ^ (key >> 22);
    return (int)key;
}

int Hash_uintptr_t(uintptr_t ptr) { return Hash_6432shift(ptr); }

int Hash_ptr(void *ptr) { return Hash_uintptr_t((uintptr_t)ptr); }
