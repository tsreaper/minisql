#include <cstring>
#include <ctime>
#include <iostream>

#include "global.h"
#include "struct/table.h"
#include "file/heapFile.h"
#include "utils/utils.h"

#include "minisql.h"
#include "catalog/catalogManager.h"
#include "record/recordManager.h"
#include "api/api.h"

using namespace std;

// Select record. Return number of records selected
int Api::select(
    const char* tableName, const vector<string>* colName,
    const vector<int>* cond, const vector<string>* operand
)
{
    // Get manager and table
    CatalogManager* catalogManager = MiniSQL::getCatalogManager();

    Table* table = catalogManager->getTable(tableName);
    if (table == NULL)
        return -1;

    int condCount = (int)cond->size();

    // Check condition validity
    for (int i = 0; i < condCount; i++)
        if (
            cond->at(i) != COND_EQ &&
            cond->at(i) != COND_NE &&
            cond->at(i) != COND_LT &&
            cond->at(i) != COND_GT &&
            cond->at(i) != COND_LE &&
            cond->at(i) != COND_GE
        )
        {
            cerr << "ERROR: [Api::select] Unknown condition `" << cond->at(i) << "`!" << endl;
            return -1;
        }

    // Check if column name exists
    for (int i = 0; i < condCount; i++)
    {
        short type = table->getType(colName->at(i).c_str());
        if (type == TYPE_NULL)
            return -1;
    }

    // Get select result
    vector<char*> record;
    vector<int> _;

    int selectCount = filter(
        tableName, colName, cond, operand, &record, &_
    );
    if (selectCount < 0)
        return -1;

    // Print column name
    cout << endl;
    int colCount = table->getColCount();
    for (int i = 0; i < colCount; i++)
        cout << table->getColName(i) << "\t";
    cout << endl << "----------------------------------------" << endl;

    // Parse and print each record
    vector<char*> parsed;

    for (auto data : record)
    {
        table->recordToVec(data, &parsed);
        for (int i = 0; i < colCount; i++)
        {
            short type = table->getType(table->getColName(i));
            
            if (type <= TYPE_CHAR)
                cout << parsed[i] << "\t";
            else if (type == TYPE_INT)
                cout << *(reinterpret_cast<int*>(parsed[i])) << "\t";
            else if (type == TYPE_FLOAT)
                cout << *(reinterpret_cast<float*>(parsed[i])) << "\t";
        }
        cout << endl;

        // Clean up parsed record
        for (int i = 0; i < colCount; i++)
            delete[] parsed[i];
        parsed.clear();
    }
    cout << endl;

    return selectCount;
}

// Insert record. Return true if success
bool Api::insert(const char* tableName, const vector<string>* value)
{
    // Get manager and table
    CatalogManager* catalogManager = MiniSQL::getCatalogManager();
    RecordManager* recordManager = MiniSQL::getRecordManager();
    IndexManager* indexManager = MiniSQL::getIndexManager();

    Table* table = catalogManager->getTable(tableName);
    if (table == NULL)
        return false;

    // Parse data
    char* data = new char[table->getRecordLength()];
    if (!table->vecToRecord(value, data))
    {
        // Parsing failed. Clean up
        delete[] data;
        return false;
    }

    // Get insert result
    int res = recordManager->insert(tableName, data);
    if (res < 0)
    {
        // Insert failed. Clean up
        delete[] data;
        return false;
    }

    // Insert data into indices
    vector<char*> vec;
    table->recordToVec(data, &vec);
    vector<Index*> indices;
    catalogManager->getIndexByTable(tableName, &indices);

    for (auto index : indices)
        indexManager->insert(
            index->getName(), vec[table->getId(index->getColName())], res
        );

    for (auto t : vec)
        delete[] t;
    delete[] data;
    return true;
}

// Delete record. Return number of records deleted
int Api::remove(
    const char* tableName, const vector<string>* colName,
    const vector<int>* cond, const vector<string>* operand
)
{
    // Get manager and table
    CatalogManager* catalogManager = MiniSQL::getCatalogManager();
    RecordManager* recordManager = MiniSQL::getRecordManager();
    IndexManager* indexManager = MiniSQL::getIndexManager();

    Table* table = catalogManager->getTable(tableName);
    if (table == NULL)
        return false;

    // Get select result
    vector<char*> record;
    vector<int> ids;

    int selectCount = filter(
        tableName, colName, cond, operand, &record, &ids
    );
    if (selectCount < 0)
        return -1;

    // Get delete result
    bool res = recordManager->remove(tableName, &ids);
    if (!res)
        return -1;

    // Delete data from indices
    vector<char*> vec;
    vector<Index*> indices;
    catalogManager->getIndexByTable(tableName, &indices);

    for (auto data : record)
    {
        vec.clear();
        table->recordToVec(data, &vec);

        for (auto index : indices)
            indexManager->remove(
                index->getName(), vec[table->getId(index->getColName())]
            );

        for (auto t : vec)
            delete[] t;
    }

    return selectCount;
}

// Create table. Return true if success
bool Api::createTable(
    const char* tableName, const char* primary,
    const vector<string>* colName, const vector<short>* colType, vector<char>* colUnique
)
{
    // Get manager
    CatalogManager* catalogManager = MiniSQL::getCatalogManager();
    RecordManager* recordManager = MiniSQL::getRecordManager();

    // Get create result
    if (catalogManager->createTable(
        tableName, primary, colName, colType, colUnique
    ))
    {
        recordManager->createTable(tableName);
        createIndex(to_string(time(0)).c_str(), tableName, primary);
        return true;
    }
    else
        return false;
}

// Drop table. Return true if success
bool Api::dropTable(const char* tableName)
{
    // Get manager
    CatalogManager* catalogManager = MiniSQL::getCatalogManager();
    RecordManager* recordManager = MiniSQL::getRecordManager();

    // Get drop result
    if (catalogManager->dropTable(tableName))
    {
        recordManager->dropTable(tableName);

        // Drop all indices related to table
        vector<Index*> indices;
        catalogManager->getIndexByTable(tableName, &indices);
        for (auto index : indices)
            dropIndex(index->getName());

        return true;
    }
    else
        return false;
}

// Create index. Return true if success
bool Api::createIndex(const char* indexName, const char* tableName, const char* colName)
{
    // Get manager
    CatalogManager* catalogManager = MiniSQL::getCatalogManager();
    IndexManager* indexManager = MiniSQL::getIndexManager();

    // Get create result
    if (catalogManager->createIndex(indexName, tableName, colName))
    {
        indexManager->createIndex(indexName);

        // Add current records into index
        HeapFile* file = new HeapFile(("record/" + string(tableName)).c_str());
        Table* table = catalogManager->getTable(tableName);
        char* data = new char[table->getRecordLength()];
        int id;
        while ((id = file->getNextRecord(data)) >= 0)
        {
            char dataOut[MAX_VALUE_LENGTH];
            table->getValue(colName, data, dataOut);
            indexManager->insert(indexName, dataOut, id);
        }

        return true;
    }
    else
        return false;
}

// Drop index. Return true if success
bool Api::dropIndex(const char* indexName)
{
    // Get manager
    CatalogManager* catalogManager = MiniSQL::getCatalogManager();
    IndexManager* indexManager = MiniSQL::getIndexManager();

    // Get drop result
    if (catalogManager->dropIndex(indexName))
    {
        indexManager->dropIndex(indexName);
        return true;
    }
    else
        return false;
}

// Filter records satisfying all conditions
// Return number of records filtered
int Api::filter(
    const char* tableName, const vector<string>* colName,
    const vector<int>* cond, const vector<string>* operand,
    vector<char*>* record, vector<int>* ids
)
{
    // Get managers
    CatalogManager* catalogManager = MiniSQL::getCatalogManager();
    RecordManager* recordManager = MiniSQL::getRecordManager();
    IndexManager* indexManager = MiniSQL::getIndexManager();

    Table* table = catalogManager->getTable(tableName);
    int condCount = (int)cond->size();

    // Try to use index
    for (int i = 0; i < condCount; i++)
    {
        if (cond->at(i) != COND_EQ)
            continue;
        Index* index = catalogManager->getIndexByTableCol(
            tableName, colName->at(i).c_str()
        );
        if (index == NULL)
            continue;

        // Use index to select
        short type = table->getType(colName->at(i).c_str());
        char* key = Utils::getDataFromStr(operand->at(i).c_str(), type);
        if (key == NULL)
            return 0;
        int id = indexManager->find(index->getName(), key);

        int ret;
        if (id < 0)
            ret = 0;
        else
        {
            // Record found. Check other conditions
            HeapFile* file = new HeapFile(("record/" + string(tableName)).c_str());
            const char* data = file->getRecordById(id);
            delete file;

            if (recordManager->checkRecord(
                data, tableName, colName, cond, operand
            ))
            {
                int recordLength = table->getRecordLength();
                char* hit = new char[recordLength];
                memcpy(hit, data, recordLength);

                record->push_back(hit);
                ids->push_back(id);
                ret = 1;
            }
            else
                ret = 0;
        }

        delete[] key;
        return ret;
    }

    // Use brute force
    return recordManager->select(
        tableName, colName, cond, operand, record, ids
    );
}
