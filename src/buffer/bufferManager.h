#ifndef _BUFFER_MANAGER_H
#define _BUFFER_MANAGER_H

#include <string>
#include <unordered_map>

#include "global.h"

using namespace std;

// Linked list node of data block. Used for LRU
struct BlockNode
{
    Block* block;
    BlockNode* pre;
    BlockNode* nxt;

    // Constructor
    BlockNode(Block* _block): block(_block) {}

    // Add to position after node
    void add(BlockNode* node)
    {
        pre = node; nxt = node->nxt;
        node->nxt->pre = this; node->nxt = this;
    }

    // Remove from linked list
    void remove() { pre->nxt = nxt; nxt->pre = pre;}

    // Destructor
    ~BlockNode() { remove();}
};

class BufferManager
{
public:

    // Max number of block
    static const int MAX_BLOCK_COUNT;

    // Constructor
    BufferManager();

    // Destructor
    ~BufferManager();

    // Get the id-th block in file
    Block* getBlock(const char* filename, int id);

    // Remove all block with filename(used when delete file)
    void removeBlockByFilename(const char* filename);

#ifdef DEBUG
    // Print block filename and id
    void debugPrint() const;
#endif

private:

    // Current block number
    int blockCnt;

    // Dummy head and tail for linked list
    BlockNode* lruHead;
    BlockNode* lruTail;

    // Block node map
    unordered_map<string, BlockNode*> nodeMap;

    // Delete node and its block from memory
    void deleteNodeBlock(BlockNode* node, bool write = true);

    // Load the id-th block from file
    Block* loadBlock(const char* filename, int id);

    // Write the id-th block of file
    void writeBlock(const char* filename, int id);
};

#endif
