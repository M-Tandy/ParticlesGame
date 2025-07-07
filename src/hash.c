#include <stdlib.h>

#include "hash.h"

int hash_6432shift(long key) {
    // From: https://gist.github.com/badboy/6267743
    key = (~key) + (key << 18); // key = (key << 18) - key - 1;
    key = key ^ (key >> 31);
    key = key * 21; // key = (key + (key << 2)) + (key << 4);
    key = key ^ (key >> 11);
    key = key + (key << 6);
    key = key ^ (key >> 22);
    return (int)key;
}

int hash_uintptr_t(uintptr_t ptr) { return hash_6432shift(ptr); }

int hash_ptr(void *ptr) { return hash_uintptr_t((uintptr_t)ptr); }
