#ifndef _INDEX_H
#define _INDEX_H

#include <string>

using namespace std;

class Index
{
public:

    // Constructor
    Index(const char* data);

    // Get index name
    const char* getName() const;

    // Get table name
    const char* getTableName() const;

    // Get column name
    const char* getColName() const;

#ifdef DEBUG
    // Print index info
    void debugPrint() const;
#endif

private:

    // Index name
    string name;

    // Table name
    string tableName;

    // Column name
    string colName;
};

#endif
