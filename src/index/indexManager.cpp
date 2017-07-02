#include <iostream>
#include <string>

#include "struct/index.h"
#include "struct/table.h"
#include "index/bpTree.h"
#include "utils/utils.h"

#include "minisql.h"
#include "catalog/catalogManager.h"
#include "index/indexManager.h"

// Find key in index. Return record id
int IndexManager::find(const char* indexName, const char* key)
{
    BPTree* tree = new BPTree(("index/" + string(indexName)).c_str());
    int ret = tree->find(key);
    delete tree;
    return ret;
}

// Insert key into index. Return true if success
bool IndexManager::insert(const char* indexName, const char* key, int value)
{
    BPTree* tree = new BPTree(("index/" + string(indexName)).c_str());
    if (!tree->add(key, value))
    {
        cerr << "ERROR: [IndexManager::insert] Duplicate key in index `" << indexName << "`." << endl;
        delete tree;
        return false;
    }
    delete tree;
    return true;
}

// Delete key from index. Return true if success
bool IndexManager::remove(const char* indexName, const char* key)
{
    BPTree* tree = new BPTree(("index/" + string(indexName)).c_str());
    if (!tree->remove(key))
    {
        cerr << "ERROR: [IndexManager::remove] Cannot find key in index `" << indexName << "`." << endl;
        delete tree;
        return false;
    }
    delete tree;
    return true;
}

// Create index. Return true if success
bool IndexManager::createIndex(const char* indexName)
{
    CatalogManager* manager = MiniSQL::getCatalogManager();
    Index* index = manager->getIndex(indexName);
    if (index == NULL)
        return false;
    Table* table = manager->getTable(index->getTableName());
    if (table == NULL)
        return false;
    int keyLength = Utils::getTypeSize(table->getType(index->getColName()));

    BPTree::createFile(("index/" + string(indexName)).c_str(), keyLength);
    return true;
}

// Drop index. Return true if success
bool IndexManager::dropIndex(const char* indexName)
{
    Utils::deleteFile(("index/" + string(indexName)).c_str());
    return true;
}
