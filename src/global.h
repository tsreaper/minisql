#ifndef _GLOBAL_H
#define _GLOBAL_H

#include <string>

using namespace std;

#define DEBUG

// Bytes of a block
#define BLOCK_SIZE 4096

// Max length of a value in a record
#define MAX_VALUE_LENGTH 256

// Max length of table/index/column name
#define MAX_NAME_LENGTH 31

// Data types
#define TYPE_NULL 0
#define TYPE_CHAR 255
#define TYPE_INT 256
#define TYPE_FLOAT 257

// Conditions
#define COND_EQ 0
#define COND_NE 1
#define COND_LT 2
#define COND_GT 3
#define COND_LE 4
#define COND_GE 5

// A data block
struct Block
{
    string filename;
    int id;

    bool dirty;
    bool pin;

    char content[BLOCK_SIZE];

    // Constructor
    Block(const char* _filename, int _id): filename(_filename), id(_id)
    {
        dirty = false;
        pin = false;
    }
};

#endif
