/**
 * this file is created by Bassant Tarek , id : 20231037
 * this file has :
 * 1- the btree node struct
 * 2- read a node function
 * 3- write a node function
 * 4- int SearchARecord (Char* filename, int RecordID) implementation
 * 5- void CreateIndexFileFile (Char* filename, int numberOfRecords, int m) implementation
 * 6- display btree visualization function
 **/

#include <bits/stdc++.h>
using namespace std;

/// BTreeNode
struct BTreeNode {
    // 0 = Leaf node (no children)
    // 1 = Internal node (has children)
    int status;

    // Array of child pointers (RRNs) for internal nodes.
    vector<int> children;

    // Array of keys or record IDs stored in this node.
    vector<int> records;

    // Number of valid keys currently in the node.
    int keyCount;

    // The Relative Record Number of this node in the file.
    int selfRRN;

    BTreeNode() {
        status = 0;             // Default: leaf node
        keyCount = 0;
        selfRRN = -1;
        children.resize(5, -1);
        records.resize(5, -1);
    }

    // Returns true if this node is a leaf, false otherwise.
    bool isLeaf() {
        return status == 0;
    }
};

/// Reads a BTreeNode from a binary file at the given RRN.

// Each node occupies: sizeof(int) (status) + 5*sizeof(int) (children) + 5*sizeof(int) (records)
const int NODE_SIZE = sizeof(int) + 5*sizeof(int) + 5*sizeof(int);
BTreeNode readNode(fstream &file, int rrn) {
    BTreeNode node;
    node.selfRRN = rrn;

    // Move the file pointer to the position of the node
    // The first int in the file is the root RRN, so we skip it: sizeof(int)
    file.seekg(sizeof(int) + rrn * NODE_SIZE, ios::beg);
    file.read(reinterpret_cast<char*>(&node.status), sizeof(int));
    file.read(reinterpret_cast<char*>(node.children.data()), 5*sizeof(int));
    file.read(reinterpret_cast<char*>(node.records.data()), 5*sizeof(int));

    node.keyCount = 0;
    for(int i=0;i<5;i++)
        if(node.records[i]!=-1)
            node.keyCount++;

    return node;
}

/// Writes a BTreeNode to a binary file at its selfRRN
void writeNode(fstream &file, BTreeNode &node) {
    file.seekp(sizeof(int) + node.selfRRN * NODE_SIZE, ios::beg);
    file.write(reinterpret_cast<char*>(&node.status), sizeof(int));
    file.write(reinterpret_cast<char*>(node.children.data()), 5*sizeof(int));
    file.write(reinterpret_cast<char*>(node.records.data()), 5*sizeof(int));

    // ensures data is written
    file.flush();
}

/// Creates a new binary B-Tree index file with 10 empty nodes
void createIndexFile(const string &fileName) {
    // Open file in binary mode, truncate if it exists
    fstream file(fileName, ios::out | ios::binary | ios::trunc);

    // Write the root RRN at the beginning of the file, we assume the root will be at RRN = 1
    int rootRRN = 1;
    file.write(reinterpret_cast<char*>(&rootRRN), sizeof(int));

    // Initialize 10 empty nodes
    for(int i = 0; i < 10; i++) {
        BTreeNode node;
        node.selfRRN = i;

        // Setup linked list of free nodes using children[0]
        if(i == 0) node.children[0] = 1;     // Node 0 points to first free node
        else if(i < 9) node.children[0] = i+1; // Each free node points to next
        else node.children[0] = -1;          // Last node points to -1 (end)

        writeNode(file, node); // Write the empty node to file
    }

    file.close();
}

/// Recursively displays the B-Tree as a visual tree
void displayTree(fstream &file, int rrn, string prefix = "", bool isLeft = true) {
    if (rrn == -1) return;

    BTreeNode node = readNode(file, rrn);

    // Print current node
    cout << prefix;

    //\-- is the branch to the last/right-most child.
    //+-- is used for all other children
    cout << (isLeft ? "+--" : "\\--");

    // Print node info
    cout << "[RRN:" << rrn << " | ";
    for (int i = 0; i < node.keyCount; i++)
        cout << node.records[i] << " ";
    cout << "]";

    if (node.isLeaf()) cout << " (leaf)";
    cout << "\n";

    // Prepare new prefix for children
    string newPrefix = prefix + (isLeft ? "|  " : "   ");

    // Recursively print children if internal
    if (!node.isLeaf())
        for (int i = 0; i <= node.keyCount; i++)
            displayTree(file, node.children[i], newPrefix, i < node.keyCount);
}

/// Searches for a record in the B-Tree index file
int SearchARecord(const char* filename, int RecordID) {
    fstream file(filename, ios::in | ios::binary);
    if(!file) return -1;

    int rootRRN;
    file.read(reinterpret_cast<char*>(&rootRRN), sizeof(int));

    int currentRRN = rootRRN;
    while(currentRRN != -1) {
        BTreeNode node = readNode(file, currentRRN);

        int i = 0;
        while(i < node.keyCount && RecordID > node.records[i])
            i++;

        if(i < node.keyCount && node.records[i] == RecordID) {
            file.close();
            return RecordID; // found
        }
        if(node.isLeaf()) {
            file.close();
            return -1; // not found
        } else {
            currentRRN = node.children[i]; // go to child
        }
    }

    file.close();
    return -1;
}

///test main to test the search function
int main() {
    const char* filename = "btree_index.bit";

    // 1. Create the index file with empty nodes
    createIndexFile(filename);
    fstream file(filename, ios::in | ios::out | ios::binary);

    // 2. Manually populate nodes to create a 4-level tree
    // Level 0: Root node (RRN = 1)
    BTreeNode root;
    root.selfRRN = 1;
    root.status = 1; // internal node
    root.records = {51, -1, -1, -1, -1};
    root.keyCount = 1;
    root.children = {2, 3, -1, -1, -1};
    writeNode(file, root);

    // Level 1
    BTreeNode node2;
    node2.selfRRN = 2;
    node2.status = 1; // internal node
    node2.records = {19, 30, -1, -1, -1};
    node2.keyCount = 2;
    node2.children = {4, 5, 6, -1, -1};
    writeNode(file, node2);

    BTreeNode node3;
    node3.selfRRN = 3;
    node3.status = 1; // internal node
    node3.records = {70, 90, -1, -1, -1};
    node3.keyCount = 2;
    node3.children = {7, 8, 9, -1, -1};
    writeNode(file, node3);

    // Level 2 (leaf nodes)
    // RRN 4 : keys < 20
    BTreeNode leaf4;
    leaf4.selfRRN = 4;
    leaf4.status = 0;
    leaf4.records = {11, 12, 13, -1, -1};
    leaf4.keyCount = 3;
    writeNode(file, leaf4);

    // RRN 5 : 20 ≤ keys < 30
    BTreeNode leaf5;
    leaf5.selfRRN = 5;
    leaf5.status = 0;
    leaf5.records = {21, 22, 23, -1, -1};
    leaf5.keyCount = 3;
    writeNode(file, leaf5);

    // RRN 6 : keys ≥ 30
    BTreeNode leaf6;
    leaf6.selfRRN = 6;
    leaf6.status = 0;
    leaf6.records = {31, 32, 33, -1, -1};
    leaf6.keyCount = 3;
    writeNode(file, leaf6);

    // RRN 7 : keys < 70
    BTreeNode leaf7;
    leaf7.selfRRN = 7;
    leaf7.status = 0;
    leaf7.records = {61, 62, 63, -1, -1};
    leaf7.keyCount = 3;
    writeNode(file, leaf7);

    // RRN 8 : 70 ≤ keys < 90
    BTreeNode leaf8;
    leaf8.selfRRN = 8;
    leaf8.status = 0;
    leaf8.records = {71, 72, 73, -1, -1};
    leaf8.keyCount = 3;
    writeNode(file, leaf8);

    // RRN 9 : keys ≥ 90
    BTreeNode leaf9;
    leaf9.selfRRN = 9;
    leaf9.status = 0;
    leaf9.records = {91, 92, 93, -1, -1};
    leaf9.keyCount = 3;
    writeNode(file, leaf9);

    // 3. Display the tree
    cout << "B-Tree structure:\n";
    displayTree(file, 1);

    // 4. Test SearchARecord
    int testKeys[] = {20, 71, 50, 90, 93,11,41,101,0,51};
    for(int key : testKeys) {
        int result = SearchARecord(filename, key);
        if(result != -1)
            cout << "Record " << key << " found in the index.\n";
        else
            cout << "Record " << key << " NOT found in the index.\n";
    }

    file.close();
}