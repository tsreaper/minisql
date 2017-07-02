#include <cstring>
#include <iostream>

#include "global.h"
#include "struct/table.h"
#include "file/heapFile.h"
#include "utils/utils.h"

#include "minisql.h"
#include "catalog/catalogManager.h"
#include "record/recordManager.h"

using namespace std;

// Select record from table. Return number of records selected
int RecordManager::select(
    const char* tableName, const vector<string>* colName,
    const vector<int>* cond, const vector<string>* operand,
    vector<char*>* record, vector<int>* ids
)
{
    // Get table and record file
    CatalogManager* manager = MiniSQL::getCatalogManager();
    Table* table = manager->getTable(tableName);
    if (table == NULL)
        return -1;

    HeapFile* file = new HeapFile(("record/" + string(tableName)).c_str());
    int recordLength = table->getRecordLength();

    // Iterate through record file
    int id, hitCount = 0;
    char* dataIn = new char[recordLength];

    while ((id = file->getNextRecord(dataIn)) >= 0)
        // Check all conditions
        if (checkRecord(
            dataIn, tableName, colName, cond, operand
        ))
        {
            char* hit = new char[recordLength];
            memcpy(hit, dataIn, recordLength);

            record->push_back(hit);
            ids->push_back(id);
            hitCount++;
        }

    delete[] dataIn;
    delete file;
    return hitCount;
}

// Insert record into table. Return new index id
int RecordManager::insert(const char* tableName, const char* data)
{
    // Get table and record file
    CatalogManager* manager = MiniSQL::getCatalogManager();
    Table* table = manager->getTable(tableName);
    if (table == NULL)
        return -1;
    HeapFile* file = new HeapFile(("record/" + string(tableName)).c_str());

    // Check unique columns
    char* exist = new char[table->getRecordLength()];
    while (file->getNextRecord(exist) >= 0)
    {
        int colId;
        if ((colId = table->checkConsistency(data, exist)) >= 0)
        {
            // Uniqueness violated! Clean up
            cerr << "ERROR: [RecordManager::insert] Duplicate values in unique column `" << table->getColName(colId) << "` of table `" << tableName << "`!" << endl;
            delete[] exist;
            delete file;
            return -1;
        }
    }

    // Insert data
    int ret = file->addRecord(data);

    delete[] exist;
    delete file;
    return ret;
}

// Delete id-th record from table. Return true if success
bool RecordManager::remove(const char* tableName, const vector<int>* ids)
{
    HeapFile* file = new HeapFile(("record/" + string(tableName)).c_str());
    for (int i = 0; i < (int)ids->size(); i++)
        file->deleteRecord(ids->at(i));
    delete file;
    return true;
}

// Create table. Return true if success
bool RecordManager::createTable(const char* tableName)
{
    CatalogManager* manager = MiniSQL::getCatalogManager();
    Table* table = manager->getTable(tableName);
    if (table == NULL)
        return false;
    HeapFile::createFile(("record/" + string(tableName)).c_str(), table->getRecordLength());
    return true;
}

// Drop table. Return true if success
bool RecordManager::dropTable(const char* tableName)
{
    Utils::deleteFile(("record/" + string(tableName)).c_str());
    return true;
}

// Check if record satisfy all conditions
bool RecordManager::checkRecord(
    const char* record, const char* tableName,
    const vector<string>* colName, const vector<int>* cond,
    const vector<string>* operand
)
{
    CatalogManager* manager = MiniSQL::getCatalogManager();
    Table* table = manager->getTable(tableName);
    if (table == NULL)
        return false;

    int condCount = colName->size();

    for (int i = 0; i < condCount; i++)
    {
        char dataOut[MAX_VALUE_LENGTH];
        short type = table->getValue(colName->at(i).c_str(), record, dataOut);

        if (type <= TYPE_CHAR)
        {
            // Char type
            if (!charCmp(dataOut, operand->at(i).c_str(), cond->at(i)))
                return false;
        }
        else if (type == TYPE_INT)
        {
            // Int type
            if (!intCmp(dataOut, operand->at(i).c_str(), cond->at(i)))
                return false;
        }
        else if (type == TYPE_FLOAT)
        {
            // Float type
            if (!floatCmp(dataOut, operand->at(i).c_str(), cond->at(i)))
                return false;
        }
    }

    return true;
}

// Compare string
bool RecordManager::charCmp(const char* a, const char* b, int op)
{
    if (op == COND_EQ)
        return strcmp(a, b) == 0;
    else if (op == COND_NE)
        return strcmp(a, b) != 0;
    else if (op == COND_LT)
        return strcmp(a, b) < 0;
    else if (op == COND_GT)
        return strcmp(a, b) > 0;
    else if (op == COND_LE)
        return strcmp(a, b) <= 0;
    else if (op == COND_GE)
        return strcmp(a, b) >= 0;
    else
        return false;
}

// Compare int
bool RecordManager::intCmp(const char* a, const char* b, int op)
{
    const int left = *(reinterpret_cast<const int*>(a));
    int right;
    sscanf(b, "%d", &right);

    if (op == COND_EQ)
        return left == right;
    else if (op == COND_NE)
        return left != right;
    else if (op == COND_LT)
        return left < right;
    else if (op == COND_GT)
        return left > right;
    else if (op == COND_LE)
        return left <= right;
    else if (op == COND_GE)
        return left >= right;
    else
        return false;
}

// Compare float
bool RecordManager::floatCmp(const char* a, const char* b, int op)
{
    const float left = *(reinterpret_cast<const float*>(a));
    float right;
    sscanf(b, "%f", &right);

    if (op == COND_EQ)
        return left == right;
    else if (op == COND_NE)
        return left != right;
    else if (op == COND_LT)
        return left < right;
    else if (op == COND_GT)
        return left > right;
    else if (op == COND_LE)
        return left <= right;
    else if (op == COND_GE)
        return left >= right;
    else
        return false;
}
