#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>

#include "global.h"
#include "minisql.h"
#include "buffer/bufferManager.h"
#include "index/bpTreeNode.h"
#include "index/bpTree.h"

using namespace std;

// States
const int BPTree::BPTREE_FAILED = -1;
const int BPTree::BPTREE_NORMAL = 0;
const int BPTree::BPTREE_ADD = 1;
const int BPTree::BPTREE_REMOVE = 2;
const int BPTree::BPTREE_CHANGE = 3;

// Create B+ Tree file
void BPTree::createFile(const char* _filename, int _keyLength, int _order)
{
    // Calculate order if not provided
    if (_order < 0)
        _order = (BLOCK_SIZE - 8) / (_keyLength + 4) + 1;

    // Create file
    FILE* file = fopen(("data/" + string(_filename) + ".mdb").c_str(), "wb");
    int header[] = {_order, _keyLength, 0, -1, -1};
    fwrite(header, 4, 5, file);
    fclose(file);
}

// Constructor
BPTree::BPTree(const char* _filename): filename(_filename)
{
    BufferManager* manager = MiniSQL::getBufferManager();
    Block* header = manager->getBlock(_filename, 0);

    // Get header information
    order = *(reinterpret_cast<int*>(header->content));
    keyLength = *(reinterpret_cast<int*>(header->content + 4));
    nodeCount = *(reinterpret_cast<int*>(header->content + 8));
    root = *(reinterpret_cast<int*>(header->content + 12));
    firstEmpty = *(reinterpret_cast<int*>(header->content + 16));

    key = new char[keyLength];
}

// Destructor
BPTree::~BPTree()
{
    delete[] key;
}

// Find value of key
int BPTree::find(const char* _key)
{
    memcpy(key, _key, keyLength);
    return root < 0 ? BPTREE_FAILED : find(root);
}

// Add key-value pair. Return true if success
bool BPTree::add(const char* _key, int _value)
{
    memcpy(key, _key, keyLength);
    value = _value;
    int res = root < 0 ? BPTREE_ADD : add(root);

    if (res == BPTREE_ADD)
    {
        // Create new root
        int newRoot = getFirstEmpty();
        BPTreeNode* node = new BPTreeNode(filename.c_str(), newRoot, keyLength, root < 0, root < 0 ? -1 : root);
        node->insert(0, key, value);
        delete node;
        root = newRoot;
    }
    updateHeader();

    return res != BPTREE_FAILED;
}

// Remove key-value pair. Return true if success
bool BPTree::remove(const char* _key)
{
    memcpy(key, _key, keyLength);
    int res = root < 0 ? false : remove(root, 0, true, NULL);
    updateHeader();
    return res != BPTREE_FAILED;
}

#ifdef DEBUG
// Print tree structure
void BPTree::debugPrint()
{
    cerr << "DEBUG: [BPTree::debugPrint] Debug print start." << endl;
    cerr << "Node number = " << nodeCount << ", first empty = " << firstEmpty << endl;
    if (root >= 0)
    {
        cerr << "Root = " << root << endl;
        debugPrint(root);
    }
    else
        cerr << "Empty tree." << endl;
    cerr << "DEBUG: [BPTree::debugPrint] Debug print end." << endl;
}
#endif

// Recursive function for finding value
int BPTree::find(int id)
{
    BPTreeNode* node = new BPTreeNode(filename.c_str(), id, keyLength);
    int pos = node->findPosition(key);

    int ret = BPTREE_FAILED;
    if (node->isLeaf())
    {
        // Check if key is found
        if (pos > 0)
        {
            const char* k = node->getKey(pos);
            if (memcmp(key, k, keyLength) == 0)
                ret = node->getPointer(pos);
        }
    }
    else
        ret = find(node->getPointer(pos));

    delete node;
    return ret;
}

// Recursive function for adding key-value pair
int BPTree::add(int id)
{
    BPTreeNode* node = new BPTreeNode(filename.c_str(), id, keyLength);
    int pos = node->findPosition(key);

    int res = node->isLeaf() ? BPTREE_ADD : add(node->getPointer(pos));
    int ret = BPTREE_NORMAL;

    if (node->isLeaf() && pos > 0)
    {
        // Check for duplicate
        const char* k = node->getKey(pos);
        if (memcmp(key, k, keyLength) == 0)
            res = BPTREE_FAILED;
    }

    if (res == BPTREE_FAILED)
        // Duplicate key
        ret = BPTREE_FAILED;
    else if (res == BPTREE_ADD)
    {
        // Add new key-value
        node->insert(pos, key, value);

        if (node->getSize() >= order)
        {
            // Node full. Split
            int newId = getFirstEmpty();
            BPTreeNode* newNode = node->split(newId, key);
            value = newId;

            delete newNode;
            ret = BPTREE_ADD;
        }
    }

    delete node;
    return ret;
}

// Recursive function for deleting key-value pair
int BPTree::remove(int id, int sibId, bool leftSib, const char* parentKey)
{
    BPTreeNode* node = new BPTreeNode(filename.c_str(), id, keyLength);
    BPTreeNode* sib = NULL;
    if (id != root)
        sib = new BPTreeNode(filename.c_str(), sibId, keyLength);
    int pos = node->findPosition(key);

    int res;
    if (node->isLeaf())
        res = BPTREE_FAILED;
    else
    {
        int nxtId = node->getPointer(pos);
        int nxtSib = node->getPointer(pos > 0 ? pos - 1 : pos + 1);
        const char* nxtParentKey = node->getKey(pos > 0 ? pos : pos + 1);
        res = remove(nxtId, nxtSib, pos > 0, nxtParentKey);
    }

    if (node->isLeaf())
    {
        // Check if key is found
        if (pos > 0)
        {
            const char* k = node->getKey(pos);
            if (memcmp(key, k, keyLength) == 0)
                res = BPTREE_REMOVE;
        }
    }

    int ret = BPTREE_NORMAL;
    if (res == BPTREE_FAILED)
        // Key not found
        ret = BPTREE_FAILED;
    else if (res == BPTREE_CHANGE)
        // Change key
        node->setKey(pos > 0 ? pos : pos + 1, key);
    else if (res == BPTREE_REMOVE)
    {
        // Delete key
        node->remove(pos > 0 ? pos : pos + 1);

        if (id == root)
        {
            if (node->getSize() == 0)
            {
                root = node->getPointer(0);
                removeBlock(id);
                node->setRemoved();
            }
        }
        else
        {
            int lim = (order+1)/2 - 1;
            if (node->getSize() < lim)
            {
                if (sib->getSize() > lim)
                {
                    // Borrow key from sibling
                    const char* k = node->borrow(sib, leftSib, parentKey);
                    memcpy(key, k, keyLength);
                    ret = BPTREE_CHANGE;
                }
                else
                {
                    // Merge sibling
                    if (leftSib)
                    {
                        sib->mergeRight(node, parentKey);
                        removeBlock(id);
                        node->setRemoved();
                    }
                    else
                    {
                        node->mergeRight(sib, parentKey);
                        removeBlock(sibId);
                        sib->setRemoved();
                    }
                    ret = BPTREE_REMOVE;
                }
            }
        }
    }

    delete node;
    if (sib != NULL)
        delete sib;
    return ret;
}

// Get first empty block id
int BPTree::getFirstEmpty()
{
    if (firstEmpty < 0)
        return ++nodeCount;

    int ret = firstEmpty;
    BufferManager* manager = MiniSQL::getBufferManager();
    Block* block = manager->getBlock(filename.c_str(), firstEmpty);
    firstEmpty = *(reinterpret_cast<int*>(block->content));
    return ret;
}

// Remove block in file
void BPTree::removeBlock(int id)
{
    BufferManager* manager = MiniSQL::getBufferManager();
    Block* block = manager->getBlock(filename.c_str(), id);
    memcpy(block->content, &firstEmpty, 4);
    firstEmpty = id;
}

// Update header information
void BPTree::updateHeader()
{
    BufferManager* manager = MiniSQL::getBufferManager();
    Block* block = manager->getBlock(filename.c_str(), 0);

    memcpy(block->content + 8, &nodeCount, 4);
    memcpy(block->content + 12, &root, 4);
    memcpy(block->content + 16, &firstEmpty, 4);

    block->dirty = true;
}

#ifdef DEBUG
// Recursive function for tree structure printing
void BPTree::debugPrint(int id)
{
    BPTreeNode* node = new BPTreeNode(filename.c_str(), id, keyLength);

    cerr << "Block id = " << id << ", isLeaf = " << node->isLeaf() << endl;
    cerr << "Keys:";
    for (int i = 1; i <= node->getSize(); i++)
    {
        cerr << " ";
        const char* k = node->getKey(i);
        for (int j = 0; j < keyLength; j++)
        {
            cerr << (int)k[j];
            if (j < keyLength-1)
                cerr << "~";
        }
    }
    cerr << endl;
    cerr << "Pointers: ";
    for (int i = 0; i <= node->getSize(); i++)
        cerr << " " << node->getPointer(i);
    cerr << endl;

    if (!node->isLeaf())
        for (int i = 0; i <= node->getSize(); i++)
            debugPrint(node->getPointer(i));

    delete node;
}
#endif
