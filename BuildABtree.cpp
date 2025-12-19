/**
 * this file is created by Bassant Tarek , id : 20231037
 * this file has :
 * 1- the btree node struct
 * 2- read a node function
 * 3- write a node function
 * 4- int SearchARecord (Char* filename, int RecordID) implementation
 * 5- void CreateIndexFileFile (Char* filename, int numberOfRecords, int m) implementation
 * 6- void DisplayIndexFileContent (Char* filename) implementation
 **/

#include <bits/stdc++.h>
using namespace std;

/// 5 keys, 5 references, 1 status
const int rowSize = 11;

/// BTreeNode structure
struct BTreeNode {
    int status;           // -1 empty, 0 leaf, 1 internal
    vector<int> keys;     // 5 keys
    vector<int> refs;     // 5 references
    int selfRRN;          // in-memory RRN

    BTreeNode() {
        status = -1;
        selfRRN = -1;
        keys.assign(5, -1);
        refs.assign(5, -1);
    }
};

/// Create empty index file with free list
void CreateIndexFile(const char* filename, int numberOfNodes) {
    ofstream file(filename, ios::binary | ios::trunc);

    vector<int> row(rowSize, -1);

    // Node 0 -> first free node = 1
    row[0] = -1;
    row[1] = 1;
    file.write(reinterpret_cast<char*>(row.data()), rowSize * sizeof(int));

    // Nodes from 1 to numberOfNodes-1 are being initialized as empty, linked list of free nodes
    for (int i = 1; i < numberOfNodes; i++) {
        row.assign(rowSize, -1);
        row[0] = -1;
        row[1] = (i + 1 < numberOfNodes) ? i + 1 : -1; // last node points to -1
        file.write(reinterpret_cast<char*>(row.data()), rowSize * sizeof(int));
    }

    file.close();
}

/// Write a node to the file (keys/refs only)
void writeNode(fstream &file, const BTreeNode &node) {
    vector<int> row(rowSize, -1);
    row[0] = node.status;

    for (int i = 0; i < 5; i++) {
        row[1 + i*2] = node.keys[i];
        row[2 + i*2] = node.refs[i];
    }

    file.seekp(node.selfRRN * rowSize * sizeof(int), ios::beg);
    file.write(reinterpret_cast<char*>(row.data()), rowSize * sizeof(int));

    //  make sure data is written correctly
    file.flush();
}

/// Read a node from the file
BTreeNode readNode(fstream &file, int rrn) {
    vector<int> row(rowSize);
    file.seekg(rrn * rowSize * sizeof(int), ios::beg);
    file.read(reinterpret_cast<char*>(row.data()), rowSize * sizeof(int));

    BTreeNode node;
    node.selfRRN = rrn;
    node.status = row[0];

    for (int i = 0; i < 5; i++) {
        node.keys[i] = row[1 + i*2];
        node.refs[i] = row[2 + i*2];
    }

    return node;
}

/// Display index file content
void DisplayIndexFileContent(const char* filename) {
    fstream file(filename, ios::in | ios::binary);
    if (!file) {
        cout << "Cannot open file\n";
        return;
    }

    for (int i = 0; i < 10; i++) {
        BTreeNode node = readNode(file, i);
        cout << node.status << " ";
        for (int j = 0; j < 5; j++)
            cout << node.keys[j] << " " << node.refs[j] << " ";
        cout << "\n";
    }

    file.close();
}

/// Search a record in the index
int SearchARecord(const char* filename, int RecordID) {
    fstream file(filename, ios::in | ios::binary);
    if (!file) return -1;

    for (int rrn = 1; rrn < 10; rrn++) {
        // skip node 0
        BTreeNode node = readNode(file, rrn);
        if (node.status == -1) continue;

        for (int i = 0; i < 5; i++) {
            if (node.keys[i] == RecordID) {
                file.close();
                return node.refs[i];
            }
        }
    }

    file.close();
    return -1;
}
