#ifndef _API_H
#define _API_H

#include <string>
#include <vector>

using namespace std;

class Api
{
public:

    // Select record. Return number of records selected
    int select(
        const char* tableName, const vector<string>* colName,
        const vector<int>* cond, const vector<string>* operand
    );

    // Insert record. Return true if success
    bool insert(const char* tableName, const vector<string>* value);

    // Delete record. Return number of records deleted
    int remove(
        const char* tableName, const vector<string>* colName,
        const vector<int>* cond, const vector<string>* operand
    );

    // Create table. Return true if success
    bool createTable(
        const char* tableName, const char* primary,
        const vector<string>* colName, const vector<short>* colType, vector<char>* colUnique
    );

    // Drop table. Return true if success
    bool dropTable(const char* tableName);

    // Create index. Return true if success
    bool createIndex(const char* indexName, const char* tableName, const char* colName);

    // Drop index. Return true if succes
    bool dropIndex(const char* indexName);

private:

    // Filter records satisfying all conditions
    // Return number of records filtered
    int filter(
        const char* tableName, const vector<string>* colName,
        const vector<int>* cond, const vector<string>* operand,
        vector<char*>* record, vector<int>* ids
    );
};

#endif
