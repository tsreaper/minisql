#include <cstring>
#include <iostream>
#include <algorithm>

#include "global.h"
#include "minisql.h"
#include "buffer/bufferManager.h"
#include "index/bpTreeNode.h"

using namespace std;

// Constructor(from file)
BPTreeNode::BPTreeNode(
    const char* _filename, int _id, int _keyLength
): filename(_filename), id(_id), keyLength(_keyLength)
{
    BufferManager* manager = MiniSQL::getBufferManager();
    Block* block = manager->getBlock(_filename, id);
    char* data = block->content;

    size = *(reinterpret_cast<int*>(data));
    keys.push_back(NULL);
    ptrs.push_back(*(reinterpret_cast<int*>(data + 4)));
    leaf = ptrs[0] < 0;
    blockRemoved = false;

    int bias = 8;
    for (int i = 1; i <= size; i++)
    {
        char* k = new char[keyLength];
        memcpy(k, data + bias, keyLength);
        keys.push_back(k);
        ptrs.push_back(*reinterpret_cast<int*>(data + bias + keyLength));
        bias += keyLength + 4;
    }
}

// Constructor(construct an empty node)
BPTreeNode::BPTreeNode(
    const char* _filename, int _id, int _keyLength, bool _leaf, int firstPtr
): filename(_filename), id(_id), keyLength(_keyLength), leaf(_leaf)
{
    size = 0;
    keys.push_back(NULL);
    ptrs.push_back(firstPtr);
    dirty = true;
    blockRemoved = false;
}

// Destructor
BPTreeNode::~BPTreeNode()
{
    // Write modification to block
    if (dirty && !blockRemoved)
    {
        BufferManager* manager = MiniSQL::getBufferManager();
        Block* block = manager->getBlock(filename.c_str(), id);
        char* data = block->content;

        // Update size
        memcpy(data, &size, 4);

        // Update first pointer
        memcpy(data + 4, &ptrs[0], 4);

        // Update key-pointer
        int bias = 8;
        for (int i = 1; i <= size; i++)
        {
            memcpy(data + bias, keys[i], keyLength);
            memcpy(data + bias + keyLength, &ptrs[i], 4);
            bias += keyLength + 4;
        }

        block->dirty = true;
    }

    // Clean up keys
    for (auto k : keys)
        if (k != NULL)
            delete[] k;
}

// Get node size
int BPTreeNode::getSize() const
{
    return size;
}

// Get key length
int BPTreeNode::getKeyLength() const
{
    return keyLength;
}

// If node is leaf
bool BPTreeNode::isLeaf() const
{
    return leaf;
}

// Get key
const char* BPTreeNode::getKey(int pos) const
{
    if (pos > size || pos <= 0)
    {
        cerr << "ERROR: [BPTreeNode::getKey] Position " << pos << " out of range!" << endl;
        return NULL;
    }

    return keys[pos];
}

// Get pointer
int BPTreeNode::getPointer(int pos) const
{
    if (pos > size || pos < 0)
    {
        cerr << "ERROR: [BPTreeNode::getKey] Position " << pos << " out of range!" << endl;
        return -1;
    }
    return ptrs[pos];
}

// Find key's position
int BPTreeNode::findPosition(const char* key) const
{
    return upper_bound(
        keys.begin() + 1, keys.end(), key,
        [&](const char* a, const char* b) { return memcmp(a, b, keyLength) < 0;}
    ) - (keys.begin() + 1);
}

// Set key at position
void BPTreeNode::setKey(int pos, const char* key)
{
    if (pos > size || pos <= 0)
    {
        cerr << "ERROR: [BPTreeNode::setKey] Position " << pos << " out of range!" << endl;
        return;
    }

    dirty = true;
    memcpy(keys[pos], key, keyLength);
}

// Set pointer at position
void BPTreeNode::setPointer(int pos, int ptr)
{
    if (pos > size || pos < 0)
    {
        cerr << "ERROR: [BPTreeNode::setPointer] Position " << pos << " out of range!" << endl;
        return;
    }

    dirty = true;
    ptrs[pos] = ptr;
}

// Set block as removed
void BPTreeNode::setRemoved()
{
    blockRemoved = true;
}

// Insert key-pointer after position
void BPTreeNode::insert(int pos, const char* key, int ptr)
{
    if (pos > size || pos < 0)
    {
        cerr << "ERROR: [BPTreeNode::insert] Position " << pos << " out of range!" << endl;
        return;
    }

    dirty = true;
    char* k = new char[keyLength];
    memcpy(k, key, keyLength);
    if (pos == size)
    {
        keys.push_back(k);
        ptrs.push_back(ptr);
    }
    else
    {
        keys.insert(keys.begin() + pos + 1, k);
        ptrs.insert(ptrs.begin() + pos + 1, ptr);
    }
    size++;
}

// Rmove key-pointer at position
void BPTreeNode::remove(int pos)
{
    if (pos > size || pos <= 0)
    {
        cerr << "ERROR: [BPTreeNode::insert] Position " << pos << " out of range!" << endl;
        return;
    }

    dirty = true;
    if (pos == size)
    {
        keys.pop_back();
        ptrs.pop_back();
    }
    else
    {
        keys.erase(keys.begin() + pos);
        ptrs.erase(ptrs.begin() + pos);
    }
    size--;
}

// Split into two nodes. Return new node
BPTreeNode* BPTreeNode::split(int newId, char* newKey)
{
    dirty = true;

    int pos = size/2 + (leaf ? 0 : 1);
    memcpy(newKey, keys[size/2 + 1], keyLength);
    BPTreeNode* ret = new BPTreeNode(filename.c_str(), newId, keyLength, leaf, leaf ? -1 : ptrs[pos]);

    // Copy former half of keys-pointers to new node
    for (pos++; pos <= size; pos++)
        ret->insert(ret->getSize(), keys[pos], ptrs[pos]);

    size /= 2;
    keys.resize(size + 1);
    ptrs.resize(size + 1);

    return ret;
}

// Borrow a key from sibling. Return new parent key
const char* BPTreeNode::borrow(BPTreeNode* sib, bool leftSib, const char* parentKey)
{
    dirty = true;

    if (leftSib)
    {
        // Borrow from left sibling
        int sibSize = sib->getSize();
        const char* sibKey = sib->getKey(sibSize);
        int sibPtr = sib->getPointer(sibSize);
        sib->remove(sibSize);

        if (leaf)
            insert(0, sibKey, sibPtr);
        else
        {
            int ptr = ptrs[0];
            ptrs[0] = sibPtr;
            insert(0, parentKey, ptr);
        }

        return sibKey;
    }
    else
    {
        // Borrow from right sibling
        const char* sibKey = sib->getKey(1);
        int sibPtr0 = sib->getPointer(0);
        int sibPtr1 = sib->getPointer(1);
        sib->remove(1);

        if (leaf)
        {
            insert(size, sibKey, sibPtr1);
            return sib->getKey(1);
        }
        else
        {
            sib->setPointer(0, sibPtr1);
            insert(size, parentKey, sibPtr0);
            return sibKey;
        }
    }
}

// Merge right sibling
void BPTreeNode::mergeRight(BPTreeNode* sib, const char* parentKey)
{
    dirty = true;

    int sibSize = sib->getSize();
    if (!leaf)
        insert(size, parentKey, sib->getPointer(0));
    for (int i = 1; i <= sibSize; i++)
        insert(size, sib->getKey(i), sib->getPointer(i));
}
