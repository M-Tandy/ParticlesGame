#ifndef ptest_hash_h
#define ptest_hash_h

#include <stdint.h>

int Hash_6432shift(long key);
int Hash_uintptr_t(uintptr_t ptr);
int Hash_ptr(void *ptr);

#endif // ptest_hash_h
