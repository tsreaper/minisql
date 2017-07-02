#ifndef _INDEX_MANAGER_H
#define _INDEX_MANAGER_H

class IndexManager
{
public:

    // Find key in index. Return record id
    int find(const char* indexName, const char* key);

    // Insert key into index. Return true if success
    bool insert(const char* indexName, const char* key, int value);

    // Delete key from index. Return true if success
    bool remove(const char* indexName, const char* key);

    // Create index. Return true if success
    bool createIndex(const char* indexName);

    // Drop index. Return true if success
    bool dropIndex(const char* indexName);
};

#endif
