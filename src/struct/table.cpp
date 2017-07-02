#include <cstdio>
#include <iostream>

#include "utils/utils.h"
#include "minisql.h"
#include "struct/table.h"

using namespace std;

// Constructor
Table::Table(const char* data)
{
    name = data;
    primary = data + MAX_NAME_LENGTH;
    colCount = 0;
    recordLength = 0;
}

// Get table name
const char* Table::getName() const
{
    return name.c_str();
}

// Get primary key name
const char* Table::getPrimary() const
{
    return primary.c_str();
}

// Get column number
int Table::getColCount()
{
    if (colCount == 0)
        loadColInfo();
    return colCount;
}

// Get length of a record
int Table::getRecordLength()
{
    if (colCount == 0)
        loadColInfo();
    return recordLength;
}

// Get column name by id
const char* Table::getColName(int id)
{
    if (colCount == 0)
        loadColInfo();
    if (id >= colCount)
    {
        cerr << "ERROR: [Table::getColName] Column id " << id << " too large!" << endl;
        return NULL;
    }
    return colNameList[id].c_str();
}

// Get id by column name
int Table::getId(const char* colName)
{
    if (colCount == 0)
        loadColInfo();
    for (int i = 0; i < colCount; i++)
        if (colNameList[i] == colName)
            return i;
    return -1;
}

// Get value by column name. Return column type
short Table::getValue(const char* colName, const char* dataIn, char* dataOut)
{
    if (colCount == 0)
        loadColInfo();

    int id = getId(colName);
    if (id < 0)
    {
        cerr << "ERROR: [Table::getValue] Table `" << name << "` has no column named `" << colName << "`!" << endl;
        return TYPE_NULL;
    }
    memcpy(dataOut, dataIn + startPos[id], Utils::getTypeSize(colType[id]));
    return colType[id];
}

// Get column type by name
short Table::getType(const char* colName)
{
    if (colCount == 0)
        loadColInfo();

    int id = getId(colName);
    if (id < 0)
    {
        cerr << "ERROR: [Table::getType] Table `" << name << "` has no column named `" << colName << "`!" << endl;
        return TYPE_NULL;
    }
    return colType[id];
}

// Get column unique by name
char Table::getUnique(const char* colName)
{
    if (colCount == 0)
        loadColInfo();

    int id = getId(colName);
    if (id < 0)
    {
        cerr << "ERROR: [Table::getUnique] Table `" << name << "` has no column named `" << colName << "`!" << endl;
        return -1;
    }
    return colUnique[id];
}

// Check consistency(uniqueness) of new data and existing data
// Return -1 if consistent, else return column id of not unique column
int Table::checkConsistency(const char* data, const char* exist)
{
    if (colCount == 0)
        loadColInfo();

    for (int i = 0; i < colCount; i++)
    {
        if (!colUnique[i])
            continue;

        int j;
        for (j = 0; j < Utils::getTypeSize(colType[i]); j++)
            if (data[startPos[i] + j] != exist[startPos[i] + j])
                break;
        if (j >= Utils::getTypeSize(colType[i]))
            return i;
    }
    return -1;
}

// Parse record to vector. Return true if success
bool Table::recordToVec(const char* data, vector<char*>* vec)
{
    if (colCount == 0)
        loadColInfo();

    for (int i = 0; i < colCount; i++)
    {
        int size = Utils::getTypeSize(colType[i]);
        char* value = new char[size];
        memcpy(value, data + startPos[i], size);
        vec->push_back(value);
    }
    return true;
}

// Parse vector to record. Return true if success
bool Table::vecToRecord(const vector<string>* vec, char* data)
{
    if (colCount == 0)
        loadColInfo();

    if ((int)vec->size() != colCount)
    {
        cerr << "ERROR: [Table::vecToRecord] Value number mismatch. Expecting " << colCount << " values, but found " << vec->size() << " values." << endl;
        return false;
    }

    for (int i = 0; i < colCount; i++)
    {
        char* key = Utils::getDataFromStr(vec->at(i).c_str(), colType[i]);
        int size = Utils::getTypeSize(colType[i]);
        
        if (key == NULL)
            return false;
        memcpy(data + startPos[i], key, size);
        delete[] key;
    }
    return true;
}

#ifdef DEBUG
void Table::debugPrint()
{
    if (colCount == 0)
        loadColInfo();

    cerr << "DEBUG: [Table::debugPrint]" << endl;
    cerr << "Table name = " << name << ", column count = " << colCount << ", primary = " << primary << ", record length = " << recordLength << endl;
    for (int i = 0; i < colCount; i++)
        cerr << "Column = " << colNameList[i] << ", type = " << colType[i] << ", unique = " << (colUnique[i] ? '1' : '0') << endl;
    cerr << "----------------------------------------" << endl;
}
#endif

// Load column info from catalog
void Table::loadColInfo()
{
    // Get column data
    colCount = MiniSQL::getCatalogManager()->loadTableColInfo(name.c_str(), &colNameList, &colType, &colUnique);

    // Calculate starting position of each column in data
    for (int i = 0; i < colCount; i++)
    {
        if (i > 0)
            startPos.push_back(startPos[i-1] + Utils::getTypeSize(colType[i-1]));
        else
            startPos.push_back(0);
    }
    recordLength = startPos[colCount-1] + Utils::getTypeSize(colType[colCount-1]);
}
