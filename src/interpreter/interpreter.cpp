#include <ctime>
#include <iostream>
#include <fstream>

#include "global.h"
#include "record/recordManager.h"
#include "interpreter/interpreter.h"

using namespace std;

// Constructor
Interpreter::Interpreter(bool _fromFile): fromFile(_fromFile)
{
    ptr = -1;
    queryCount = 0;
    exiting = false;
    tokenizer = new Tokenizer();
    api = new Api();
}

// Destructor
Interpreter::~Interpreter()
{
    delete tokenizer;
    delete api;
}

// Get total number of queries processed
int Interpreter::getQueryCount() const
{
    return queryCount;
}

// If user is exiting mini SQL
bool Interpreter::isExiting() const
{
    return exiting;
}

// If all the tokens in vector is executed
bool Interpreter::tokenVecEmpty() const
{
    return ptr == (int)tokens.size() - 1;
}

// Execute SQL statement
void Interpreter::execute(const char* sql)
{
    int endCount = tokenizer->getTokens(sql, &tokens, &type);
    queryCount += endCount;

    while (endCount--)
    {
        ptr++;
        if (type[ptr] != Tokenizer::TOKEN_IDENTIFIER && type[ptr] != Tokenizer::TOKEN_END)
            reportUnexpected("execute", "instruction");
        else if (type[ptr] == Tokenizer::TOKEN_END) {}
        else if (tokens[ptr] == "select")
            select();
        else if (tokens[ptr] == "insert")
            insert();
        else if (tokens[ptr] == "delete")
            remove();
        else if (tokens[ptr] == "create")
            create();
        else if (tokens[ptr] == "drop")
            drop();
        else if (tokens[ptr] == "exec" || tokens[ptr] == "execfile")
            execfile();
        else if (tokens[ptr] == "exit" || tokens[ptr] == "quit")
            exit();
        else
        {
            cerr << "ERROR: [Interpreter::execute] Unknown instruction '" << tokens[ptr] << "'." << endl;
            skipStatement();
        }
    }
}

// Deal with select
void Interpreter::select()
{
    ptr++;
    if (tokens[ptr] != "*" || type[ptr] != Tokenizer::TOKEN_SYMBOL)
    {
        reportUnexpected("select", "'*'(MiniSQL does not support selecting specific columns)");
        return;
    }

    ptr++;
    if (tokens[ptr] != "from" || type[ptr] != Tokenizer::TOKEN_IDENTIFIER)
    {
        reportUnexpected("select", "'from'");
        return;
    }

    ptr++;
    if (type[ptr] != Tokenizer::TOKEN_IDENTIFIER)
    {
        reportUnexpected("select", "table name");
        return;
    }

    // Prepare select information
    const char* tableName = tokens[ptr].c_str();
    vector<string> colName;
    vector<int> cond;
    vector<string> operand;

    if (where(&colName, &cond, &operand))
    {
        // Do selection
        int tic, toc, selectCount;
        tic = clock();
        selectCount = api->select(tableName, &colName, &cond, &operand);
        toc = clock();

        // Print execution time
        if (selectCount >= 0 && !fromFile)
            cout << selectCount << " record(s) selected. Query done in " << 1.0 * (toc-tic) / CLOCKS_PER_SEC << "s." << endl;
    }
}

// Deal with insert
void Interpreter::insert()
{
    ptr++;
    if (tokens[ptr] != "into" || type[ptr] != Tokenizer::TOKEN_IDENTIFIER)
    {
        reportUnexpected("insert", "'into'");
        return;
    }

    ptr++;
    if (type[ptr] != Tokenizer::TOKEN_IDENTIFIER)
    {
        reportUnexpected("insert", "table name");
        return;
    }

    // Prepare insert information
    const char* tableName = tokens[ptr].c_str();
    vector<string> value;

    ptr++;
    if (tokens[ptr] != "values" || type[ptr] != Tokenizer::TOKEN_IDENTIFIER)
    {
        reportUnexpected("insert", "'values'");
        return;
    }

    ptr++;
    if (tokens[ptr] != "(" || type[ptr] != Tokenizer::TOKEN_SYMBOL)
    {
        reportUnexpected("insert", "'('");
        return;
    }

    while (true)
    {
        ptr++;
        if (type[ptr] != Tokenizer::TOKEN_NUMBER && type[ptr] != Tokenizer::TOKEN_STRING_SINGLE && type[ptr] != Tokenizer::TOKEN_STRING_DOUBLE)
        {
            reportUnexpected("insert", "value");
            return;
        }
        value.push_back(tokens[ptr]);

        ptr++;
        if (tokens[ptr] == ")" && type[ptr] == Tokenizer::TOKEN_SYMBOL)
            break;
        else if (tokens[ptr] != "," || type[ptr] != Tokenizer::TOKEN_SYMBOL)
        {
            reportUnexpected("insert", "','");
            return;
        }
    }

    ptr++;
    if (type[ptr] != Tokenizer::TOKEN_END)
    {
        reportUnexpected("insert", "';'");
        return;
    }

    // Do insertion
    int tic, toc;
    bool res;
    tic = clock();
    res = api->insert(tableName, &value);
    toc = clock();

    // Print execution time
    if (res && !fromFile)
        cout << "1 record inserted. Query done in " << 1.0 * (toc-tic) / CLOCKS_PER_SEC << "s." << endl;
}

// Deal with delete
void Interpreter::remove()
{
    ptr++;
    if (tokens[ptr] != "from" || type[ptr] != Tokenizer::TOKEN_IDENTIFIER)
    {
        reportUnexpected("delete", "'from'");
        return;
    }

    ptr++;
    if (type[ptr] != Tokenizer::TOKEN_IDENTIFIER)
    {
        reportUnexpected("delete", "table name");
        return;
    }

    // Prepare delete information
    const char* tableName = tokens[ptr].c_str();
    vector<string> colName;
    vector<int> cond;
    vector<string> operand;

    if (where(&colName, &cond, &operand))
    {
        // Do deletion
        int tic, toc, removeCount;
        tic = clock();
        removeCount = api->remove(tableName, &colName, &cond, &operand);
        toc = clock();

        // Print execution time
        if (removeCount >= 0 && !fromFile)
            cout << removeCount << " record(s) deleted. Query done in " << 1.0 * (toc-tic) / CLOCKS_PER_SEC << "s." << endl;
    }
}

// Deal with where. Return true if success
bool Interpreter::where(vector<string>* colName, vector<int>* cond, vector<string>* operand)
{
    ptr++;
    if (type[ptr] == Tokenizer::TOKEN_END)
        // No condition
        return true;
    else if (tokens[ptr] != "where" || type[ptr] != Tokenizer::TOKEN_IDENTIFIER)
    {
        reportUnexpected("select", "'where'");
        return false;
    }

    // With condition
    while (true)
    {
        ptr++;
        if (type[ptr] != Tokenizer::TOKEN_IDENTIFIER)
        {
            reportUnexpected("select", "column name");
            return false;
        }
        colName->push_back(tokens[ptr]);

        ptr++;
        if (type[ptr] != Tokenizer::TOKEN_OPERATOR)
        {
            reportUnexpected("select", "operator");
            return false;
        }
        int op = getOperatorType(tokens[ptr].c_str());
        if (op < 0)
        {
            cerr << "ERROR: [Interpreter::select] Unknown operator '" << tokens[ptr] << "'." << endl;
            skipStatement();
            return false;
        }
        cond->push_back(op);

        ptr++;
        if (type[ptr] != Tokenizer::TOKEN_NUMBER && type[ptr] != Tokenizer::TOKEN_STRING_SINGLE && type[ptr] != Tokenizer::TOKEN_STRING_DOUBLE)
        {
            reportUnexpected("select", "value");
            return false;
        }
        operand->push_back(tokens[ptr]);

        ptr++;
        if (type[ptr] == Tokenizer::TOKEN_END)
            return true;
        else if (tokens[ptr] != "and" || type[ptr] != Tokenizer::TOKEN_IDENTIFIER)
        {
            reportUnexpected("select", "'and'(MiniSQL only supports conjunctive selection)");
            return false;
        }
    }
}

// Deal with creata table/index
void Interpreter::create()
{
    ptr++;
    if (tokens[ptr] == "table" && type[ptr] == Tokenizer::TOKEN_IDENTIFIER)
        createTable();
    else if (tokens[ptr] == "index" && type[ptr] == Tokenizer::TOKEN_IDENTIFIER)
        createIndex();
    else
        reportUnexpected("create", "'table' or 'index'");
}

// Deal with create table
void Interpreter::createTable()
{
    ptr++;
    if (type[ptr] != Tokenizer::TOKEN_IDENTIFIER)
    {
        reportUnexpected("createTable", "table name");
        return;
    }

    // Prepare create table information
    const char* tableName = tokens[ptr].c_str();
    const char* primary = NULL;
    vector<string> colName;
    vector<short> colType;
    vector<char> colUnique;

    ptr++;
    if (tokens[ptr] != "(" || type[ptr] != Tokenizer::TOKEN_SYMBOL)
    {
        reportUnexpected("createTable", "'('");
        return;
    }

    while (true)
    {
        ptr++;
        if (tokens[ptr] == "primary" && type[ptr] == Tokenizer::TOKEN_IDENTIFIER)
        {
            bool hasBracket = false;

            ptr++;
            if (tokens[ptr] != "key" || type[ptr] != Tokenizer::TOKEN_IDENTIFIER)
            {
                reportUnexpected("createTable", "'key'");
                return;
            }

            ptr++;
            if (tokens[ptr] == "(" && type[ptr] == Tokenizer::TOKEN_SYMBOL)
                hasBracket = true;
            else if (type[ptr] == Tokenizer::TOKEN_IDENTIFIER)
            {
                if (primary != NULL)
                {
                    cerr << "ERROR: [Interpreter::createTable] Multiple primary key definition." << endl;
                    skipStatement();
                    return;
                }
                primary = tokens[ptr].c_str();
            }
            else
            {
                reportUnexpected("createTable", "primary key name or '('");
                return;
            }

            if (hasBracket)
            {
                ptr++;
                if (type[ptr] != Tokenizer::TOKEN_IDENTIFIER)
                {
                    reportUnexpected("createTable", "primary key name");
                    return;
                }
                else if (primary != NULL)
                {
                    cerr << "ERROR: [Interpreter::createTable] Multiple primary key definition." << endl;
                    skipStatement();
                    return;
                }
                primary = tokens[ptr].c_str();

                ptr++;
                if (tokens[ptr] != ")" || type[ptr] != Tokenizer::TOKEN_SYMBOL)
                {
                    reportUnexpected("createTable", "')'");
                    return;
                }
            }
        }
        else if (type[ptr] == Tokenizer::TOKEN_IDENTIFIER)
        {
            colName.push_back(tokens[ptr]);

            int t = getNextColType();
            if (t == TYPE_NULL)
                return;
            colType.push_back(t);

            if (tokens[ptr+1] == "unique" && type[ptr+1] == Tokenizer::TOKEN_IDENTIFIER)
            {
                ptr++;
                colUnique.push_back(1);
            }
            else
                colUnique.push_back(0);
        }
        else
        {
            reportUnexpected("createTable", "column name or 'primary'");
            return;
        }

        ptr++;
        if (tokens[ptr] == ")" && type[ptr] == Tokenizer::TOKEN_SYMBOL)
            break;
        else if (tokens[ptr] != "," || type[ptr] != Tokenizer::TOKEN_SYMBOL)
        {
            reportUnexpected("createTable", "','");
            return;
        }
    }

    ptr++;
    if (type[ptr] != Tokenizer::TOKEN_END)
    {
        reportUnexpected("createTable", "';'");
        return;
    }

    if (primary == NULL)
    {
        cerr << "ERROR: [Interpreter::createTable] No primary key definition!" << endl;
        return;
    }

    // Do creation
    int tic, toc;
    bool res;
    tic = clock();
    res = api->createTable(tableName, primary, &colName, &colType, &colUnique);
    toc = clock();
    
    // Print execution time
    if (res && !fromFile)
        cout << "1 table created. Query done in " << 1.0 * (toc-tic) / CLOCKS_PER_SEC << "s." << endl;
}

// Deal with create index
void Interpreter::createIndex()
{
    ptr++;
    if (type[ptr] != Tokenizer::TOKEN_IDENTIFIER)
    {
        reportUnexpected("createIndex", "index name");
        return;
    }

    // Prepare create table information
    const char* indexName = tokens[ptr].c_str();
    const char* tableName;
    const char* colName;

    ptr++;
    if (tokens[ptr] != "on" || type[ptr] != Tokenizer::TOKEN_IDENTIFIER)
    {
        reportUnexpected("createIndex", "'on'");
        return;
    }

    ptr++;
    if (type[ptr] != Tokenizer::TOKEN_IDENTIFIER)
    {
        reportUnexpected("createIndex", "index name");
        return;
    }
    tableName = tokens[ptr].c_str();

    ptr++;
    if (tokens[ptr] != "(" || type[ptr] != Tokenizer::TOKEN_SYMBOL)
    {
        reportUnexpected("createIndex", "'('");
        return;
    }

    ptr++;
    if (type[ptr] != Tokenizer::TOKEN_IDENTIFIER)
    {
        reportUnexpected("createIndex", "column name");
        return;
    }
    colName = tokens[ptr].c_str();

    ptr++;
    if (tokens[ptr] != ")" || type[ptr] != Tokenizer::TOKEN_SYMBOL)
    {
        reportUnexpected("createIndex", "')'");
        return;
    }

    ptr++;
    if (type[ptr] != Tokenizer::TOKEN_END)
    {
        reportUnexpected("createIndex", "';'");
        return;
    }

    // Do creation
    int tic, toc;
    bool res;
    tic = clock();
    res = api->createIndex(indexName, tableName, colName);
    toc = clock();
    
    // Print execution time
    if (res && !fromFile)
        cout << "1 index created. Query done in " << 1.0 * (toc-tic) / CLOCKS_PER_SEC << "s." << endl;
}

// Deal with drop table/index
void Interpreter::drop()
{
    ptr++;
    if (tokens[ptr] == "table" && type[ptr] == Tokenizer::TOKEN_IDENTIFIER)
    {
        ptr++;
        if (type[ptr] != Tokenizer::TOKEN_IDENTIFIER)
        {
            reportUnexpected("drop", "table name");
            return;
        }
        const char* tableName = tokens[ptr].c_str();

        ptr++;
        if (type[ptr] != Tokenizer::TOKEN_END)
        {
            reportUnexpected("drop", "';'");
            return;
        }

        // Do drop table
        int tic, toc;
        bool res;
        tic = clock();
        res = api->dropTable(tableName);
        toc = clock();

        // Print execution time
        if (res && !fromFile)
            cout << "1 table dropped. Query done in " << 1.0 * (toc-tic) / CLOCKS_PER_SEC << "s." << endl;
    }
    else if (tokens[ptr] == "index" && type[ptr] == Tokenizer::TOKEN_IDENTIFIER)
    {
        ptr++;
        if (type[ptr] != Tokenizer::TOKEN_IDENTIFIER)
        {
            reportUnexpected("drop", "index name");
            return;
        }
        const char* indexName = tokens[ptr].c_str();

        ptr++;
        if (type[ptr] != Tokenizer::TOKEN_END)
        {
            reportUnexpected("drop", "';'");
            return;
        }

        // Do drop index
        int tic, toc;
        bool res;
        tic = clock();
        res = api->dropIndex(indexName);
        toc = clock();

        // Print execution time
        if (res && !fromFile)
            cout << "1 index dropped. Query done in " << 1.0 * (toc-tic) / CLOCKS_PER_SEC << "s." << endl;
    }
    else
        reportUnexpected("drop", "'table' or 'index'");
}

// Deal with execfile
void Interpreter::execfile()
{
    ptr++;
    if (type[ptr] != Tokenizer::TOKEN_STRING_SINGLE && type[ptr] != Tokenizer::TOKEN_STRING_DOUBLE)
    {
        reportUnexpected("execfile", "a string as filename");
        return;
    }
    const char* filename = tokens[ptr].c_str();

    ptr++;
    if (type[ptr] != Tokenizer::TOKEN_END)
    {
        reportUnexpected("execfile", "';'");
        return;
    }

    if (fromFile)
    {
        cerr << "ERROR: [Interpreter::execfile] Cannot do 'execfile' instruction when executing from file." << endl;
        return;
    }

    // Read from file
    ifstream file(filename);
    if (!file.is_open())
    {
        cerr << "ERROR: [Interpreter::execfile] Cannot load file " << filename << "!" << endl;
        return;
    }
    string line, content = "";
    while (!file.eof())
    {
        getline(file, line);
        content += line + '\n';
    }

    // Execute file
    int tic, toc;
    Interpreter* interpreter = new Interpreter(true);
    tic = clock();
    interpreter->execute(content.c_str());
    toc = clock();

    // Print execution time
    cout << interpreter->getQueryCount() << " queries done in " << 1.0 * (toc-tic) / CLOCKS_PER_SEC << "s." << endl;

    delete interpreter;
}

// Deal with exit
void Interpreter::exit()
{
    ptr++;
    if (type[ptr] != Tokenizer::TOKEN_END)
    {
        reportUnexpected("exit", "';'");
        return;
    }

    if (fromFile)
        cerr << "ERROR: [Interpreter::exit] Cannot do 'exit' instruction when executing from file." << endl;
    else
    {
        cout << "Bye~ :)" << endl;
        exiting = true;
    }
}

// Get operator type
int Interpreter::getOperatorType(const char* op)
{
    string s = op;
    if (s == "=")
        return COND_EQ;
    else if (s == "<>")
        return COND_NE;
    else if (s == "<")
        return COND_LT;
    else if (s == ">")
        return COND_GT;
    else if (s == "<=")
        return COND_LE;
    else if (s == ">=")
        return COND_GE;
    else
        return -1;
}

// Get next column type
short Interpreter::getNextColType()
{
    ptr++;
    if (tokens[ptr] == "char" && type[ptr] == Tokenizer::TOKEN_IDENTIFIER)
    {
        ptr++;
        if (tokens[ptr] != "(" || type[ptr] != Tokenizer::TOKEN_SYMBOL)
        {
            reportUnexpected("getNextColType", "'('");
            return TYPE_NULL;
        }

        ptr++;
        int len = stoi(tokens[ptr]);
        if (type[ptr] != Tokenizer::TOKEN_NUMBER || len <= 0 || len > TYPE_CHAR)
        {
            reportUnexpected("getNextColType", "1~255");
            return TYPE_NULL;
        }

        ptr++;
        if (tokens[ptr] != ")" || type[ptr] != Tokenizer::TOKEN_SYMBOL)
        {
            reportUnexpected("getNextColType", "')'");
            return TYPE_NULL;
        }

        return len;
    }
    else if (tokens[ptr] == "int" && type[ptr] == Tokenizer::TOKEN_IDENTIFIER)
        return TYPE_INT;
    else if (tokens[ptr] == "float" && type[ptr] == Tokenizer::TOKEN_IDENTIFIER)
        return TYPE_FLOAT;
    else
    {
        reportUnexpected("getNextColType", "'char', 'int' or 'float'(MiniSQL only supports these three data types)");
        return TYPE_NULL;
    }
}

// Report unexpected error
void Interpreter::reportUnexpected(const char* position, const char* expecting)
{
    cerr << "ERROR: [Interpreter::" << position << "] Expecting " << expecting << ", but found '" << tokens[ptr] << "'." << endl;
    skipStatement();
}

// Skip current statement
void Interpreter::skipStatement()
{
    if (ptr < 0)
        ptr = 0;
    for (; type[ptr] != Tokenizer::TOKEN_END; ptr++);
}
