#ifndef _BPTREE_H
#define _BPTREE_H

#include <vector>
#include <string>

using namespace std;

class BPTree
{
public:

    // Create B+ tree file
    static void createFile(const char* _filename, int _keyLength, int _order = -1);

    // Constructor
    BPTree(const char* _filename);

    // Destructor
    ~BPTree();

    // Find value of key
    int find(const char* _key);

    // Add key-value pair. Return true if success
    bool add(const char* _key, int _value);

    // Remove key-value pair. Return true if success
    bool remove(const char* _key);

#ifdef DEBUG
    // Print tree structure
    void debugPrint();
#endif

private:

    // States
    static const int BPTREE_FAILED;
    static const int BPTREE_NORMAL;
    static const int BPTREE_ADD;
    static const int BPTREE_REMOVE;
    static const int BPTREE_CHANGE;

    // Order of tree
    int order;

    // Length of each key
    int keyLength;

    // Total number of nodes
    int nodeCount;

    // Block id of root
    int root;

    // First empty block in file
    int firstEmpty;

    // Binary file name
    string filename;

    // Key-value to maintain
    char* key;
    int value;

    // Recursive function for finding value
    int find(int id);

    // Recursive function for adding key-value pair
    int add(int id);

    // Recursive function for deleting key-value pair
    int remove(int id, int sibId, bool leftSib, const char* parentKey);

    // Get first empty block id
    int getFirstEmpty();

    // Remove block in file
    void removeBlock(int id);

    // Update header information
    void updateHeader();

#ifdef DEBUG
    // Recursive function for tree structure printing
    void debugPrint(int id);
#endif
};

#endif
