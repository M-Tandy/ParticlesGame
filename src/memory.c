#include <stdlib.h>

#include "memory.h"

// Reallocate (grow, or shrink!) the memory in the pointer from oldSize to newSize.
// Only use reallocate to allocate or free memory so we can keep track of memory use.
void *reallocate(void *pointer, size_t oldSize, size_t newSize) {
    if (newSize == 0) {
        free(pointer);
        return NULL;
    }

    // realloc will return the same pointer in the case of growing and the array can be grown
    // successfully from its current contiguous block to a new contiguous block. Otherwise, it
    // copies the data to a new contiguous block supporting the new size, and returns a pointer to
    // that.
    void *result = realloc(pointer, newSize);

    if (result == NULL) {
        // Memory has ran out!
        exit(1);
    }

    return result;
}
