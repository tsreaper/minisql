#ifndef _UTILS_H
#define _UTILS_H

class Utils
{
public:

    // Get data size of each type
    static int getTypeSize(short type);
    
    // Check if a file exists
    static bool fileExists(const char* filename);

    // Delete file
    static void deleteFile(const char* filename);
    
    // Parse string to binary data according to type
    static char* getDataFromStr(const char* s, int type);
};

#endif
