#include <cstdio>
#include <iostream>
#include "buffer/bufferManager.h"

using namespace std;

// Max number of block
const int BufferManager::MAX_BLOCK_COUNT = 100;

// Constructor
BufferManager::BufferManager()
{
    blockCnt = 0;

    // Initialize dummy head and tail
    lruHead = new BlockNode(NULL);
    lruTail = new BlockNode(NULL);
    lruHead->pre = lruHead->nxt = lruTail;
    lruTail->pre = lruTail->nxt = lruHead;
}

// Destructor
BufferManager::~BufferManager()
{
    // Clean up linked list
    while (lruHead->nxt != lruTail)
        deleteNodeBlock(lruHead->nxt);
    delete lruHead;
    delete lruTail;
}

// Get the id-th block in file
Block* BufferManager::getBlock(const char* filename, int id)
{
    string blockName = string(filename) + "`" + to_string(id);

    if (nodeMap.find(blockName) != nodeMap.end())
    {
        // Set block as most recently used
        BlockNode* node = nodeMap[blockName];
        node->remove();
        node->add(lruHead);
        return node->block;
    }

    if (blockCnt == MAX_BLOCK_COUNT)
    {
        // Current block number full
        // Find the least recently used block
        BlockNode* node = lruTail->pre;
        while (node->block->pin)
            node = node->pre;

        deleteNodeBlock(node);
    }

    return loadBlock(filename, id);
}

// Remove all block with filename(used when delete file)
void BufferManager::removeBlockByFilename(const char* filename)
{
    BlockNode* nxtNode;
    for (BlockNode* node = lruHead->nxt; node != lruTail; node = nxtNode)
    {
        nxtNode = node->nxt;
        if (node->block->filename == filename)
            deleteNodeBlock(node, false);
    }
}

#ifdef DEBUG
// Print block filename and id
void BufferManager::debugPrint() const
{
    BlockNode* node = lruHead->nxt;
    cerr << "DEBUG: [BufferManager::debugPrint]" << endl;
    for (; node != lruTail; node = node->nxt)
        cerr << "Block filename = " << node->block->filename << ", id = " << node->block->id << endl;
    cerr << "----------------------------------------" << endl;
}
#endif

// Delete node and its block from memory
void BufferManager::deleteNodeBlock(BlockNode* node, bool write)
{
    Block* block = node->block;
    writeBlock(block->filename.c_str(), block->id);
    nodeMap.erase(block->filename + "`" + to_string(block->id));
    delete node;
    delete block;
    blockCnt--;
}

// Load the id-th block from file
Block* BufferManager::loadBlock(const char* filename, int id)
{
    // Load block from file
    Block* block = new Block(filename, id);
    FILE* file = fopen(("data/" + string(filename) + ".mdb").c_str(), "rb");
    fseek(file, id*BLOCK_SIZE, SEEK_SET);
    fread(block->content, BLOCK_SIZE, 1, file);
    fclose(file);

    // Add block to linked list
    BlockNode* node = new BlockNode(block);
    node->add(lruHead);
    nodeMap[string(filename) + "`" + to_string(id)] = node;
    blockCnt++;

    return block;
}

// Write the id-th block of file
void BufferManager::writeBlock(const char* filename, int id)
{
    Block* block = nodeMap[string(filename) + "`" + to_string(id)]->block;
    if (block->dirty == false)
        return;

    FILE* file = fopen(("data/" + string(filename) + ".mdb").c_str(), "rb+");
    fseek(file, id*BLOCK_SIZE, SEEK_SET);
    fwrite(block->content, BLOCK_SIZE, 1, file);
    fclose(file);
}
