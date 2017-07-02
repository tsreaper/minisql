#ifndef _RECORD_MANAGER_H
#define _RECORD_MANAGER_H

#include <vector>
#include <string>

using namespace std;

class RecordManager
{
public:

    // Select record from table. Return number of records selected
    int select(
        const char* tableName, const vector<string>* colName,
        const vector<int>* cond, const vector<string>* operand,
        vector<char*>* record, vector<int>* ids
    );

    // Insert record into table. Return new index id
    int insert(const char* tableName, const char* data);

    // Delete id-th record from table. Return true if success
    bool remove(const char* tableName, const vector<int>* ids);

    // Create table. Return true if success
    bool createTable(const char* tableName);

    // Drop table. Return true if success
    bool dropTable(const char* tableName);

    // Check if record satisfy all conditions
    bool checkRecord(
        const char* record, const char* tableName,
        const vector<string>* colName, const vector<int>* cond,
        const vector<string>* operand
    );

private:

    // Compare string
    bool charCmp(const char* a, const char* b, int op);

    // Compare int
    bool intCmp(const char* a, const char* b, int op);

    // Compare float
    bool floatCmp(const char* a, const char* b, int op);
};

#endif
