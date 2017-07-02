#include <cstring>
#include <iostream>
#include <string>

#include "global.h"
#include "minisql.h"
#include "utils/utils.h"

using namespace std;

// Get data size of each type
int Utils::getTypeSize(short type)
{
    if (type == TYPE_NULL)
        return 0;
    else if (type < TYPE_CHAR)
        return type + 1;
    else if (type == TYPE_INT)
        return 4;
    else if (type == TYPE_FLOAT)
        return 4;
    else
    {
        cerr << "ERROR: [Utils::getTypeSize] Unknown type " << type << "!" << endl;
        return 0;
    }
}

// Check if a file exists
bool Utils::fileExists(const char* filename)
{
    FILE* file = fopen(("data/" + string(filename) + ".mdb").c_str(), "rb");
    if (file)
    {
        fclose(file);
        return true;
    }
    return false;
}

// Delete file
void Utils::deleteFile(const char* filename)
{
    remove(("data/" + string(filename) + ".mdb").c_str());
    MiniSQL::getBufferManager()->removeBlockByFilename(filename);
}

// Parse string to binary data according to type
char* Utils::getDataFromStr(const char* s, int type)
{
    char* key = NULL;
    int size = getTypeSize(type);
    
    if (type <= TYPE_CHAR)
    {
        int len = strlen(s);
        if (len > type)
        {
            cerr << "ERROR: [Utils::getDataFromStr] Expecting char(" << type << "), but found char(" << len << ")." << endl;
            return NULL;
        }
        
        key = new char[size]();
        memcpy(key, s, min(size, len + 1));
    }
    else if (type == TYPE_INT)
    {
        int value;
        if (sscanf(s, "%d", &value) < 1)
        {
            cerr << "ERROR: [Utils::getDataFromStr] Expecting int, but found '" << s << "'." << endl;
            return NULL;
        }
        
        key = new char[size]();
        memcpy(key, &value, size);
    }
    else if (type == TYPE_FLOAT)
    {
        float value;
        if (sscanf(s, "%f", &value) < 1)
        {
            cerr << "ERROR: [Utils::getDataFromStr] Expecting float, but found '" << s << "'." << endl;
            return NULL;
        }
        
        key = new char[size]();
        memcpy(key, &value, size);
    }
    
    return key;
}