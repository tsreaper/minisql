#ifndef _CATALOG_MANAGER_H
#define _CATALOG_MANAGER_H

#include <vector>
#include <string>
#include <unordered_map>

#include "file/heapFile.h"
#include "struct/table.h"
#include "struct/index.h"

using namespace std;

class CatalogManager
{
public:

    // Constructor
    CatalogManager();

    // Destructor
    ~CatalogManager();

    // Get table by name
    Table* getTable(const char* tableName) const;

    // Create table. Return true if success
    bool createTable(
        const char* tableName, const char* primary,
        const vector<string>* colName, const vector<short>* colType, vector<char>* colUnique
    );

    // Drop table. Return true if success
    bool dropTable(const char* tableName);

    // Get index by name
    Index* getIndex(const char* indexName) const;

    // Get all indices by table name
    void getIndexByTable(const char* tableName, vector<Index*>* vec);

    // Get index by table name and column name
    Index* getIndexByTableCol(const char* tableName, const char* colName);

    // Create index. Return true if success
    bool createIndex(const char* indexName, const char* tableName, const char* colName);

    // Drop index. Return true if success
    bool dropIndex(const char* indexName);

    // Load table column info. Returns column number
    int loadTableColInfo(
        const char* tableName, vector<string>* colNameData,
        vector<short>* colType, vector<char>* colUnique
    );

#ifdef DEBUG
    // Print all tables and indices info
    void debugPrint() const;
#endif

private:

    // Table map
    unordered_map<string, Table*> tableMap;
    unordered_map<string, int> tableIdMap;

    // Index name map
    unordered_map<string, Index*> indexMap;
    unordered_map<string, int> indexIdMap;

    // Table name file
    HeapFile* tableNameFile;

    // Index meta data file
    HeapFile* indexMetaFile;
};

#endif
