#include <cstring>
#include <iostream>
#include <unordered_set>

#include "global.h"
#include "utils/utils.h"
#include "catalog/catalogManager.h"

using namespace std;

// Constructor
CatalogManager::CatalogManager()
{
    // Load catalog file
    tableNameFile = new HeapFile("catalog/tables");
    indexMetaFile = new HeapFile("catalog/indices");

    // Read catalog file
    int id;
    char tableData[MAX_NAME_LENGTH*2];
    char indexData[MAX_NAME_LENGTH*3];
    while ((id = tableNameFile->getNextRecord(tableData)) >= 0)
    {
        Table* table = new Table(tableData);
        tableIdMap[table->getName()] = id;
        tableMap[table->getName()] = table;
    }
    while ((id = indexMetaFile->getNextRecord(indexData)) >= 0)
    {
        Index* index = new Index(indexData);
        indexIdMap[index->getName()] = id;
        indexMap[index->getName()] = index;
    }
}

// Destructor
CatalogManager::~CatalogManager()
{
    // Clean up table and index pointers
    for (auto table : tableMap)
        delete table.second;
    for (auto index : indexMap)
        delete index.second;

    // Clean up catalog file pointers
    delete tableNameFile;
    delete indexMetaFile;
}

// Get table by name
Table* CatalogManager::getTable(const char* tableName) const
{
    if (tableMap.find(tableName) == tableMap.end())
    {
        cerr << "ERROR: [CatalogManager::getTable] Table `" << tableName << "` does not exist!" << endl;
        return NULL;
    }
    return tableMap.at(tableName);
}

// Create table. Return true if success
bool CatalogManager::createTable(
    const char* tableName, const char* primary,
    const vector<string>* colName, const vector<short>* colType, vector<char>* colUnique
)
{
    if (tableMap.find(tableName) != tableMap.end())
    {
        cerr << "ERROR: [CatalogManager::createTable] Table `" << tableName << "` already exists!" << endl;
        return false;
    }

    int colCount = colName->size();

    // Make sure each column name is unique
    unordered_set<string> colNameSet;
    for (auto name : *colName)
    {
        if (colNameSet.find(name) != colNameSet.end())
        {
            cerr << "ERROR: [CatalogManager::createTable] Duplicate column name `" << name << "`!" << endl;
            return false;
        }
        colNameSet.insert(name);
    }

    // Make sure primary key is among column names
    if (colNameSet.find(primary) == colNameSet.end())
    {
        cerr << "ERROR: [CatalogManager::createTable] Cannot find primary key name `" << primary << "` in column names!" << endl;
        return false;
    }

    // Set primary key to unique
    for (int i = 0; i < colCount; i++)
        if (primary == colName->at(i))
            colUnique->at(i) = 1;

    // Write table data to catalog file
    char tableData[MAX_NAME_LENGTH*2];
    memcpy(tableData, tableName, MAX_NAME_LENGTH);
    memcpy(tableData + MAX_NAME_LENGTH, primary, MAX_NAME_LENGTH);
    int id = tableNameFile->addRecord(tableData);

    // Record table to map
    tableIdMap[tableName] = id;
    tableMap[tableName] = new Table(tableData);

    // Create table column data file
    HeapFile::createFile(("catalog/table_" + string(tableName)).c_str(), MAX_NAME_LENGTH + 3);
    HeapFile* tableDataFile = new HeapFile(("catalog/table_" + string(tableName)).c_str());

    for (int i = 0; i < colCount; i++)
    {
        char colData[MAX_NAME_LENGTH + 3];
        memcpy(colData, colName->at(i).c_str(), MAX_NAME_LENGTH);
        memcpy(colData + MAX_NAME_LENGTH, &(colType->at(i)), 2);
        memcpy(colData + MAX_NAME_LENGTH + 2, &(colUnique->at(i)), 1);
        tableDataFile->addRecord(colData);
    }

    return true;
}

// Drop table. Return true if success
bool CatalogManager::dropTable(const char* tableName)
{
    if (tableMap.find(tableName) == tableMap.end())
    {
        cerr << "ERROR: [CatalogManager::dropTable] Table `" << tableName << "` does not exist!" << endl;
        return false;
    }

    // Delete table
    delete tableMap[tableName];
    tableNameFile->deleteRecord(tableIdMap[tableName]);
    tableIdMap.erase(tableName);
    tableMap.erase(tableName);

    // Delete table column data file
    Utils::deleteFile(("catalog/table_" + string(tableName)).c_str());

    return true;
}

// Get index by name
Index* CatalogManager::getIndex(const char* indexName) const
{
    if (indexMap.find(indexName) == indexMap.end())
    {
        cerr << "ERROR: [CatalogManager::getIndex] Index `" << indexName << "` does not exist!" << endl;
        return NULL;
    }
    return indexMap.at(indexName);
}

// Get all indices by table name
void CatalogManager::getIndexByTable(const char* tableName, vector<Index*>* vec)
{
    for (auto item : indexMap)
    {
        Index* index = item.second;
        if (strcmp(index->getTableName(), tableName) == 0)
            vec->push_back(index);
    }
}

// Get index by table name and column name
Index* CatalogManager::getIndexByTableCol(const char* tableName, const char* colName)
{
    for (auto item : indexMap)
    {
        Index* index = item.second;
        if (
            strcmp(index->getTableName(), tableName) == 0 &&
            strcmp(index->getColName(), colName) == 0
        )
            return index;
    }
    return NULL;
}

// Create index. Return true if success
bool CatalogManager::createIndex(const char* indexName, const char* tableName, const char* colName)
{
    if (indexMap.find(indexName) != indexMap.end())
    {
        cerr << "ERROR: [CatalogManager::createIndex] Index `" << indexName << "` already exists!" << endl;
        return false;
    }

    if (tableMap.find(tableName) == tableMap.end())
    {
        cerr << "ERROR: [CatalogManager::createIndex] Table `" << tableName << "` does not exist!" << endl;
        return false;
    }

    // Check if colName exists, and if column is unique
    Table* table = tableMap[tableName];
    char isUnique = table->getUnique(colName);
    if (isUnique == 0)
        cerr << "ERROR: [CatalogManager::createIndex] Column `" << colName << "` is not unique!" << endl;
    if (isUnique != 1)
        return false;

    // Check if there is already an index with same table name and column name
    Index* exist = getIndexByTableCol(tableName, colName);
    if (exist != NULL)
    {
        cerr << "ERROR: [CatalogManager::createIndex] Index with table name `" << tableName << "` and column name `" << colName << "` already exists(Index name `" << exist->getName() << "`)!" << endl;
        return false;
    }

    // Write index data to catalog file
    char indexData[MAX_NAME_LENGTH*3];
    memcpy(indexData, indexName, MAX_NAME_LENGTH);
    memcpy(indexData + MAX_NAME_LENGTH, tableName, MAX_NAME_LENGTH);
    memcpy(indexData + MAX_NAME_LENGTH*2, colName, MAX_NAME_LENGTH);
    int id = indexMetaFile->addRecord(indexData);

    // Record index into map
    indexIdMap[indexName] = id;
    indexMap[indexName] = new Index(indexData);

    return true;
}

// Drop index. Return true if success
bool CatalogManager::dropIndex(const char* indexName)
{
    if (indexMap.find(indexName) == indexMap.end())
    {
        cerr << "ERROR: [CatalogManager::dropIndex] Index `" << indexName << "` does not exist!" << endl;
        return false;
    }

    // Delete index
    delete indexMap[indexName];
    indexMetaFile->deleteRecord(indexIdMap[indexName]);
    indexIdMap.erase(indexName);
    indexMap.erase(indexName);

    return true;
}

// Load table column info. Returns column number
int CatalogManager::loadTableColInfo(
    const char* tableName, vector<string>* colName,
    vector<short>* colType, vector<char>* colUnique
)
{
    HeapFile* colFile = new HeapFile(("catalog/table_" + string(tableName)).c_str());

    int colCount = colFile->getRecordCount();
    char colData[MAX_NAME_LENGTH + 3];
    for (int i = 0; i < colCount; i++)
    {
        colFile->getNextRecord(colData);

        char nameData[MAX_NAME_LENGTH];
        memcpy(nameData, colData, MAX_NAME_LENGTH);
        colName->push_back(nameData);

        short type = *(reinterpret_cast<short*>(colData + MAX_NAME_LENGTH));
        colType->push_back(type);

        char unique = colData[MAX_NAME_LENGTH + 2];
        colUnique->push_back(unique);
    }

    delete colFile;
    return colCount;
}

#ifdef DEBUG
// Print all tables and indices info
void CatalogManager::debugPrint() const
{
    cerr << "DEBUG: [CatalogManager::debugPrint] debugPrint begin" << endl;
    for (auto table : tableMap)
        table.second->debugPrint();
    for (auto index : indexMap)
        index.second->debugPrint();
    cerr << "DEBUG: [CatalogManager::debugPrint] debugPrint end" << endl;
}
#endif
