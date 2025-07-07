#ifndef ptest_hash_h
#define ptest_hash_h

#include <stdint.h>

int hash_6432shift(long key);
int hash_uintptr_t(uintptr_t ptr);
int hash_ptr(void *ptr);

#endif // ptest_hash_h
