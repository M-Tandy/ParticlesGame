#include "table.h"

#include "common.h"
#include "memory.h"
#include "quadtree.h"

void initTable(Table *table) {
    table->count = 0;
    table->capacity = 0;
    table->entries = NULL;
}

void freeTable(Table *table) {
    FREE_ARRAY(Entry, table->entries, table->capacity);
    initTable(table);
}

static Entry *findEntry(Entry *entries, int capacity, uint32_t key) {
    uint32_t index = key % capacity;

    for (;;) {
        Entry *entry = &entries[index];
        if (entry->key == -1) {
            // Empty entry
            return entry;
        } else if (entry->key == key) {
            return entry;
        }

        index = (index + 1) % capacity;
    }
}

// Returns `true` if a the key is found in the table, and stores its value in `out`
bool tableGet(Table *table, uint32_t key, QuadTree *out) {
    if (table->count == 0) {
        return false;
    }

    Entry *entry = findEntry(table->entries, table->capacity, key);
    // If the entries bucket is empty the key will be 0
    if (entry->key == -1) {
        return false;
    }

    *out = *entry->value;
    return true;
}

static void adjustCapacity(Table *table, int capacity) {
    Entry *entries = ALLOCATE(Entry, capacity);
    for (int i = 0; i < capacity; i++) {
        entries[i].key = -1;
        entries[i].value = NULL;
    }

    for (int i = 0; i < table->capacity; i++) {
        Entry *entry = &table->entries[i];
        if (entry->key == -1)
            continue;

        Entry *dest = findEntry(entries, capacity, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;
        table->count++;
    }

    FREE_ARRAY(Entry, table->entries, table->capacity);
    table->entries = entries;
    table->capacity = capacity;
}

bool tableSet(Table *table, uint32_t key, QuadTree *value) {
    if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
        int capacity = GROW_CAPACITY(table->capacity);
        adjustCapacity(table, capacity);
    }

    Entry *entry = findEntry(table->entries, table->capacity, key);
    bool isNewKey = entry->key == -1;
    if (isNewKey) {
        table->count++;
    }

    entry->key = key;
    entry->value = value;
    return isNewKey;
}

void tableAddAll(Table *from, Table *to) {
    for (int i = 0; i < from->capacity; i++) {
        Entry *entry = &from->entries[i];
        if (entry->key != -1) {
            tableSet(to, entry->key, entry->value);
        }
    }
}

QuadTree *tableFindQuadTree(Table *table, QuadTree *quadtree, uint32_t hash) {
    if (table->count == 0) {
        return NULL;
    }

    uint32_t index = hash % table->capacity;
    for (;;) {
        Entry *entry = &table->entries[index];
        if (entry->key == -1) {
            return NULL;
        } else if (entry->key == hash && quadtreesEqual(entry->value, quadtree)) {
            return entry->value;
        }

        index = (index + 1) % table->capacity;
    }
}
