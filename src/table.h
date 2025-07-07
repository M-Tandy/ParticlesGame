// TODO: Implement tombstones

#ifndef ptest_table_h
#define ptest_table_h

#include <quadtree.h>
#include <stdint.h>

#define TABLE_MAX_LOAD 0.75

typedef struct Entry {
    uint32_t key;
    QuadTree *value;
} Entry;

typedef struct Table {
    int count;
    int capacity;
    Entry *entries;
} Table;

void initTable(Table *table);
void freeTable(Table *table);
bool tableGet(Table *table, uint32_t key, QuadTree *out);
bool tableSet(Table *table, uint32_t key, QuadTree *value);
void tableAddAll(Table *from, Table *to);
QuadTree *tableFindQuadTree(Table *table, QuadTree *quadtree, uint32_t hash);

#endif // !ptest_table_h
