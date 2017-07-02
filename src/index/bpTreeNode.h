#ifndef _BPTREE_NODE_H
#define _BPTREE_NODE_H

#include <vector>
#include <string>

using namespace std;

class BPTreeNode
{
public:

    // Constructor
    BPTreeNode(const char* _filename, int _id, int _keyLength);
    BPTreeNode(const char* _filename, int _id, int _keyLength, bool _leaf, int firstPtr);

    // Destructor
    ~BPTreeNode();

    // Get node size
    int getSize() const;

    // Get key length
    int getKeyLength() const;

    // If node is leaf
    bool isLeaf() const;

    // Get key
    const char* getKey(int pos) const;

    // Get pointer
    int getPointer(int pos) const;

    // Find key's position
    int findPosition(const char* key) const;

    // Set key at position
    void setKey(int pos, const char* key);

    // Set pointer at position
    void setPointer(int pos, int ptr);

    // Set the block as removed
    void setRemoved();

    // Insert key-pointer after position
    void insert(int pos, const char* key, int ptr);

    // Remove key-pointer at position
    void remove(int pos);

    // Split into two nodes. Return new node
    BPTreeNode* split(int newId, char* newKey);

    // Borrow a key from sibling. Return new parent key
    const char* borrow(BPTreeNode* sib, bool leftSib, const char* parentKey);

    // Merge right sibling
    void mergeRight(BPTreeNode* sib, const char* parentKey);

private:

    // Node filename
    string filename;

    // Block id in file
    int id;

    // Node size
    int size;

    // Length of each key
    int keyLength;

    // If node is leaf
    bool leaf;

    // If node has been modified
    bool dirty;

    // If block has been removed
    bool blockRemoved;
    
    // Node keys
    vector<char*> keys;

    // Node pointers
    vector<int> ptrs;

};

#endif
