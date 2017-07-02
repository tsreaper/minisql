#ifndef _TABLE_H
#define _TABLE_H

#include <vector>
#include <string>

#include "global.h"

using namespace std;

class Table
{
public:

    // Constructor
    Table(const char* data);

    // Get table name
    const char* getName() const;

    // Get primary key name
    const char* getPrimary() const;

    // Get column number
    int getColCount();

    // Get length of a record
    int getRecordLength();

    // Get column name by id
    const char* getColName(int id);

    // Get id by column name
    int getId(const char* colName);

    // Get value by column name. Return column type
    short getValue(const char* colName, const char* dataIn, char* dataOut);

    // Get column type by name
    short getType(const char* colName);

    // Get column unique by name
    char getUnique(const char* colName);

    // Check consistency(uniqueness) of new data and existing data
    // Return -1 if consistent, else return column id of not unique column
    int checkConsistency(const char* data, const char* exist);

    // Parse record to vector. Return true if success
    bool recordToVec(const char* data, vector<char*>* vec);

    // Parse vector to record. Return true if success
    bool vecToRecord(const vector<string>* vec, char* data);

#ifdef DEBUG
    // Print table info
    void debugPrint();
#endif

private:

    // Table name
    string name;

    // Primary key
    string primary;

    // Column number
    int colCount;

    // Length of a record
    int recordLength;

    // Column name
    vector<string> colNameList;

    // Column type
    vector<short> colType;

    // Are columns unique
    vector<char> colUnique;

    // Starting position of each column in data
    vector<int> startPos;

    // Load column info from catalog
    void loadColInfo();
};

#endif
