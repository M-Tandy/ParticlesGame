#ifndef ptest_memory_h
#define ptest_memory_h

#include "common.h"

#define ALLOCATE(type, count) (type *)reallocate(NULL, 0, sizeof(type) * (count))

#define FREE(type, pointer) reallocate(pointer, sizeof(type), 0)

// If the array is empty we will default to a size of 8. Otherwise, double capcity when growing.
#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity)*2)

// Grow the array of the given type in the pointer from oldCount number of elements to newCount
// number of elements
#define GROW_ARRAY(type, pointer, oldCount, newCount)                                                                  \
    (type *)reallocate(pointer, sizeof(type) * (oldCount), sizeof(type) * (newCount))

#define FREE_ARRAY(type, pointer, oldCount) reallocate(pointer, sizeof(type) * (oldCount), 0);

void *reallocate(void *pointer, size_t oldSize, size_t newSize);
void freeObjects();

#endif // !ptest_memory_h

