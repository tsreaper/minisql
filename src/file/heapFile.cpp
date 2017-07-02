#include <cstdio>
#include <cstring>
#include <iostream>

#include "buffer/bufferManager.h"
#include "minisql.h"
#include "file/heapFile.h"

using namespace std;

// File begin indicator
const int HeapFile::FILE_BEGIN = -1;

// Create heap file
void HeapFile::createFile(const char* _filename, int _recordLength)
{
    // Add validity byte at the end
    _recordLength++;

    FILE* file = fopen(("data/" + string(_filename) + ".mdb").c_str(), "wb");
    char data[BLOCK_SIZE] = {0};
    memcpy(data, &_recordLength, 4);
    // Set record count to 0
    memset(data + 4, 0, 4);
    // Set first empty record to -1
    memset(data + 8, 0xFF, 4);
    fwrite(data, BLOCK_SIZE, 1, file);
    fclose(file);
}

// Constructor
HeapFile::HeapFile(const char* _filename): filename(_filename)
{
    BufferManager* manager = MiniSQL::getBufferManager();
    block = manager->getBlock(_filename, 0);

    // Read file header
    recordLength = *(reinterpret_cast<int*>(block->content));
    recordCount = *(reinterpret_cast<int*>(block->content + 4));
    firstEmpty = *(reinterpret_cast<int*>(block->content + 8));

    // Calculate extra information
    recordBlockCount = BLOCK_SIZE / recordLength;
    ptr = -1;
}

// Get record number
int HeapFile::getRecordCount() const
{
    return recordCount;
}

// Read next record. Return id of the record
int HeapFile::getNextRecord(char* data)
{
    bool invalid = true;

    // Read next valid record
    do
    {
        if (ptr + 1 >= recordCount)
        {
            // End of file
            memset(data, 0, sizeof(char) * (recordLength-1));
            return -1;
        }

        loadRecord(ptr + 1);
        invalid = *(reinterpret_cast<char*>(block->content + bias + recordLength - 1));
    }
    while (invalid);

    memcpy(data, block->content + bias, recordLength-1);
    return ptr;
}

// Read id-th record
const char* HeapFile::getRecordById(int id)
{
    if (id >= recordCount)
        return NULL;

    loadRecord(id);
    bool invalid = *(reinterpret_cast<char*>(block->content + bias + recordLength - 1));
    if (invalid)
        return NULL;

    return block->content + bias;
}

// Add record into file. Return id of the record
int HeapFile::addRecord(const char* data)
{
    loadRecord(firstEmpty >= 0 ? firstEmpty : recordCount);

    if (firstEmpty >= 0)
        // Update first empty record
        firstEmpty = *(reinterpret_cast<int*>(block->content + bias));
    else
        // Update total number of records
        recordCount++;

    // Update data
    memcpy(block->content + bias, data, recordLength-1);
    memset(block->content + bias + recordLength - 1, 0, 1);
    block->dirty = true;

    updateHeader();
    return ptr;
}

// Delete the id-th record. Return true if success
bool HeapFile::deleteRecord(int id)
{
    if (id >= recordCount)
    {
        cerr << "ERROR: [HeapFile::deleteRecord] Index out of range!" << endl;
        return false;
    }

    // Check record validity
    loadRecord(id);
    bool invalid = *(reinterpret_cast<char*>(block->content + bias + recordLength - 1));
    if (invalid)
    {
        cerr << "ERROR: [HeapFile::deleteRecord] Record already deleted!" << endl;
        return false;
    }

    // Update data
    memcpy(block->content + bias, &firstEmpty, 4);
    memset(block->content + bias + recordLength - 1, 1, 1);
    block->dirty = true;

    firstEmpty = ptr;
    updateHeader();
    return true;
}

// Move pointer to after the id-th record
void HeapFile::moveTo(int id)
{
    ptr = id;
}

// Update file header
void HeapFile::updateHeader()
{
    BufferManager* manager = MiniSQL::getBufferManager();
    Block* header = manager->getBlock(filename.c_str(), 0);
    memcpy(header->content + 4, &recordCount, 4);
    memcpy(header->content + 8, &firstEmpty, 4);
    header->dirty = true;
}

// Load id-th record to block
void HeapFile::loadRecord(int id)
{
    BufferManager* manager = MiniSQL::getBufferManager();
    ptr = id;
    block = manager->getBlock(filename.c_str(), ptr / recordBlockCount + 1);
    bias = ptr % recordBlockCount * recordLength;
}
