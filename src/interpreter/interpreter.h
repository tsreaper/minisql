#ifndef _INTERPRETER_H
#define _INTERPRETER_H

#include <vector>
#include <string>

#include "interpreter/tokenizer.h"
#include "api/api.h"

using namespace std;

class Interpreter
{
public:

    // Constructor
    Interpreter(bool _fromFile = false);

    // Destructor
    ~Interpreter();

    // Get total number of queries processed
    int getQueryCount() const;

    // If user is exiting mini SQL
    bool isExiting() const;

    // If all the tokens in vector is executed
    bool tokenVecEmpty() const;

    // Execute SQL statement
    void execute(const char* sql);

private:

    // Token vector
    vector<string> tokens;

    // Token type vector
    vector<int> type;

    // Current token pointer;
    int ptr;

    // Total number of queries processed
    int queryCount;

    // If user is exiting mini SQL
    bool exiting;

    // If is parsing SQL statement from file
    bool fromFile;

    // Tokenizer
    Tokenizer* tokenizer;

    // Api
    Api* api;

    // Deal with select
    void select();

    // Deal with insert
    void insert();

    // Deal with delete
    void remove();

    // Deal with where. Return true if success
    bool where(vector<string>* colName, vector<int>* cond, vector<string>* operand);

    // Deal with create table/index
    void create();

    // Deal with create table
    void createTable();

    // Deal with create index
    void createIndex();

    // Deal with drop table/index
    void drop();

    // Deal with execfile
    void execfile();

    // Deal with exit
    void exit();

    // Get operator type
    int getOperatorType(const char* op);

    // Get next column type
    short getNextColType();

    // Report unexpected error
    void reportUnexpected(const char* position, const char* expecting);

    // Skip current statement
    void skipStatement();
};

#endif
