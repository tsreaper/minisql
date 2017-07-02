#include <iostream>

#include "global.h"
#include "struct/index.h"

using namespace std;

// Constructor
Index::Index(const char* data)
{
    name = data;
    tableName = data + MAX_NAME_LENGTH;
    colName = data + MAX_NAME_LENGTH*2;
}

// Get index name
const char* Index::getName() const
{
    return name.c_str();
}

// Get table name
const char* Index::getTableName() const
{
    return tableName.c_str();
}

// Get column name
const char* Index::getColName() const
{
    return colName.c_str();
}

#ifdef DEBUG
// Print index info
void Index::debugPrint() const
{
    cerr << "DEBUG: [Index::debugPrint]" << endl;
    cerr << "Index name = " << name << ", table name = " << tableName << ", column name = " << colName << endl;
    cerr << "----------------------------------------" << endl;
}
#endif
