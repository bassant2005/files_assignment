#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>

/// this file is created by: Nour Hany Salem , id : 20230447

using namespace std;

// --- Constants ---
const int M = 5;
const int ROW_SIZE = 11; // Matches Build.cpp

struct RecordEntry {
    int key;
    int reference;
    bool operator<(const RecordEntry &other) const {
        return key < other.key;
    }
};

// --- Raw Buffer Helpers ---
void ReadNodeRaw(const char *filename, int nodeIndex, int *buffer) {
    ifstream file(filename, ios::binary);
    file.seekg(nodeIndex * ROW_SIZE * sizeof(int), ios::beg);
    file.read(reinterpret_cast<char *>(buffer), ROW_SIZE * sizeof(int));
    file.close();
}

void WriteNodeRaw(const char *filename, int nodeIndex, int *buffer) {
    ofstream file(filename, ios::binary | ios::in | ios::out);
    file.seekp(nodeIndex * ROW_SIZE * sizeof(int), ios::beg);
    file.write(reinterpret_cast<char *>(buffer), ROW_SIZE * sizeof(int));
    file.close();
}

int GetFreeNode(const char *filename) {
    int *buffer = new int[ROW_SIZE];

    // Read Header (Node 0)
    ReadNodeRaw(filename, 0, buffer);
    int freeNode = buffer[1];

    if (freeNode == -1) {
        delete[] buffer;
        return -1; // Disk Full
    }

    // Read the free node to find the next one
    int *freeNodeBuff = new int[ROW_SIZE];
    ReadNodeRaw(filename, freeNode, freeNodeBuff);
    int nextFree = freeNodeBuff[1];

    // Update Header
    buffer[1] = nextFree;
    WriteNodeRaw(filename, 0, buffer);

    // Clean the allocated node
    for (int i = 0; i < ROW_SIZE; i++) freeNodeBuff[i] = -1;
    freeNodeBuff[0] = 0; // Default to Leaf status
    WriteNodeRaw(filename, freeNode, freeNodeBuff);

    delete[] buffer;
    delete[] freeNodeBuff;
    return freeNode;
}

// --- Propagation Helper ---
void propagateMaxKeyUpdate(const char* filename, const vector<int>& path, int childRRN, int newMax) {
    int currentChildRRN = childRRN;
    int currentMax = newMax;

    for (int i = path.size() - 2; i >= 0; i--) {
        int parentRRN = path[i];
        int *parentBuf = new int[ROW_SIZE];
        ReadNodeRaw(filename, parentRRN, parentBuf);

        bool updated = false;
        bool isLastKey = false;

        for (int k = 1; k < ROW_SIZE; k += 2) {
            if (parentBuf[k+1] == currentChildRRN) {
                if (parentBuf[k] != currentMax) {
                    parentBuf[k] = currentMax;
                    updated = true;
                }
                if (k + 2 >= ROW_SIZE || parentBuf[k+2] == -1) {
                    isLastKey = true;
                }
                break;
            }
        }

        if (updated) WriteNodeRaw(filename, parentRRN, parentBuf);
        delete[] parentBuf;

        if (!updated || !isLastKey) return;
        currentChildRRN = parentRRN;
    }
}

// --- Recursive Internal Insert Function ---
bool insertIntoInternal(const char *filename, int parentRRN, int upKey, int upRef, vector<int> &path) {
    int *parentBuf = new int[ROW_SIZE];
    ReadNodeRaw(filename, parentRRN, parentBuf);

    vector<RecordEntry> entries;
    for (int i = 1; i < ROW_SIZE; i += 2) {
        if (parentBuf[i] != -1) {
            entries.push_back({parentBuf[i], parentBuf[i + 1]});
        }
    }

    entries.push_back({upKey, upRef});
    sort(entries.begin(), entries.end());

    // 1: Fits in Node
    if (entries.size() <= M) {
        for (int i = 1; i < ROW_SIZE; i++) parentBuf[i] = -1;
        parentBuf[0] = 1;
        int idx = 1;
        for (const auto &entry : entries) {
            parentBuf[idx++] = entry.key;
            parentBuf[idx++] = entry.reference;
        }
        WriteNodeRaw(filename, parentRRN, parentBuf);
        delete[] parentBuf;

        if (!path.empty() && entries.back().key == upKey) {
            propagateMaxKeyUpdate(filename, path, parentRRN, upKey);
        }
        return true;
    }

    // 2: Split Internal Node
    int mid = entries.size() / 2;
    int maxLeft = entries[mid - 1].key;
    int maxRight = entries.back().key;

    // --- FIX: Check Root Split FIRST ---
    if (parentRRN == 1) {
        // Root Split: Allocate Left (2) then Right (3)
        int leftNodeIndex = GetFreeNode(filename);
        int rightNodeIndex = GetFreeNode(filename);

        // Prepare Left Node (contains first half)
        int *leftBuf = new int[ROW_SIZE];
        for (int k = 0; k < ROW_SIZE; k++) leftBuf[k] = -1;
        leftBuf[0] = 1; // Internal
        int idx = 1;
        for(int i=0; i<mid; i++) {
            leftBuf[idx++] = entries[i].key;
            leftBuf[idx++] = entries[i].reference;
        }
        WriteNodeRaw(filename, leftNodeIndex, leftBuf);
        delete[] leftBuf;

        // Prepare Right Node (contains second half)
        int *rightBuf = new int[ROW_SIZE];
        for (int k = 0; k < ROW_SIZE; k++) rightBuf[k] = -1;
        rightBuf[0] = 1; // Internal
        idx = 1;
        for(size_t i=mid; i<entries.size(); i++) {
            rightBuf[idx++] = entries[i].key;
            rightBuf[idx++] = entries[i].reference;
        }
        WriteNodeRaw(filename, rightNodeIndex, rightBuf);
        delete[] rightBuf;

        // Update Root (1) to point to Left (2) and Right (3)
        int *rootBuf = new int[ROW_SIZE];
        for (int k = 0; k < ROW_SIZE; k++) rootBuf[k] = -1;
        rootBuf[0] = 1; // Internal
        rootBuf[1] = maxLeft; rootBuf[2] = leftNodeIndex;
        rootBuf[3] = maxRight; rootBuf[4] = rightNodeIndex;

        WriteNodeRaw(filename, 1, rootBuf);
        delete[] parentBuf; delete[] rootBuf;
        return true;
    }

    // --- Normal Internal Split (Not Root) ---
    int rightNodeIndex = GetFreeNode(filename);

    int *rightBuf = new int[ROW_SIZE];
    for (int k = 0; k < ROW_SIZE; k++) rightBuf[k] = -1;
    rightBuf[0] = 1; // Internal

    int idx = 1;
    for (size_t i = mid; i < entries.size(); i++) {
        rightBuf[idx++] = entries[i].key;
        rightBuf[idx++] = entries[i].reference;
    }
    WriteNodeRaw(filename, rightNodeIndex, rightBuf);

    // Update Current (Left)
    for (int i = 1; i < ROW_SIZE; i++) parentBuf[i] = -1;
    parentBuf[0] = 1;
    idx = 1;
    for (int i = 0; i < mid; i++) {
        parentBuf[idx++] = entries[i].key;
        parentBuf[idx++] = entries[i].reference;
    }
    WriteNodeRaw(filename, parentRRN, parentBuf);

    // Recursive up
    delete[] parentBuf;
    delete[] rightBuf;

    if (!path.empty()) path.pop_back();
    if (path.empty()) return false;

    int grandparentRRN = path.back();

    // Update key for Left Node in Grandparent
    int *gpBuf = new int[ROW_SIZE];
    ReadNodeRaw(filename, grandparentRRN, gpBuf);
    for (int i = 1; i < ROW_SIZE; i += 2) {
        if (gpBuf[i+1] == parentRRN) {
            gpBuf[i] = maxLeft;
            break;
        }
    }
    WriteNodeRaw(filename, grandparentRRN, gpBuf);
    delete[] gpBuf;

    return insertIntoInternal(filename, grandparentRRN, maxRight, rightNodeIndex, path);
}

// --- Main Insert Function ---
int InsertNewRecordAtIndex(const char *filename, int RecordID, int Reference) {
    int *buffer = new int[ROW_SIZE];

    // 1. Initialize Root
    ReadNodeRaw(filename, 1, buffer);
    if (buffer[0] == -1) {
        int *header = new int[ROW_SIZE];
        ReadNodeRaw(filename, 0, header);
        header[1] = buffer[1];
        WriteNodeRaw(filename, 0, header);
        delete[] header;

        for (int k = 0; k < ROW_SIZE; k++) buffer[k] = -1;
        buffer[0] = 0; // Leaf
        buffer[1] = RecordID;
        buffer[2] = Reference;
        WriteNodeRaw(filename, 1, buffer);
        delete[] buffer;
        return 1;
    }

    // 2. Traverse
    int currentNode = 1;
    vector<int> path;

    while (true) {
        ReadNodeRaw(filename, currentNode, buffer);
        path.push_back(currentNode);

        if (buffer[0] == 0) break; // Leaf

        bool found = false;
        for (int i = 1; i < ROW_SIZE; i += 2) {
            if (buffer[i] != -1 && RecordID <= buffer[i]) {
                currentNode = buffer[i + 1];
                found = true;
                break;
            }
        }
        if (!found) {
            for (int i = ROW_SIZE - 2; i >= 1; i -= 2) {
                if (buffer[i] != -1) {
                    currentNode = buffer[i + 1];
                    found = true;
                    break;
                }
            }
        }
        if (!found) { delete[] buffer; return -1; }
    }

    // 3. Insert into Leaf
    vector<RecordEntry> entries;
    for (int i = 1; i < ROW_SIZE; i += 2) {
        if (buffer[i] != -1) {
            entries.push_back({buffer[i], buffer[i + 1]});
        }
    }
    entries.push_back({RecordID, Reference});
    sort(entries.begin(), entries.end());

    if (entries.size() <= M) {
        for (int i = 1; i < ROW_SIZE; i++) buffer[i] = -1;
        int idx = 1;
        for (const auto &entry : entries) {
            buffer[idx++] = entry.key;
            buffer[idx++] = entry.reference;
        }
        WriteNodeRaw(filename, currentNode, buffer);

        if (entries.back().key == RecordID) {
            propagateMaxKeyUpdate(filename, path, currentNode, RecordID);
        }

        delete[] buffer;
        return currentNode;
    } else {
        // --- Leaf Split Logic ---
        int mid = entries.size() / 2;
        int maxLeft = entries[mid - 1].key;
        int maxRight = entries.back().key;

        // --- FIX: Check for Root Split FIRST ---
        if (path.size() == 1) { // Root is the only node in path
            // Allocate Left (2) then Right (3)
            int leftNodeIndex = GetFreeNode(filename);
            int rightNodeIndex = GetFreeNode(filename);

            // Write Left Node (2)
            int *leftBuf = new int[ROW_SIZE];
            for(int k=0; k<ROW_SIZE; k++) leftBuf[k] = -1;
            leftBuf[0] = 0; // Leaf
            int idx = 1;
            for(int i=0; i<mid; i++) {
                leftBuf[idx++] = entries[i].key;
                leftBuf[idx++] = entries[i].reference;
            }
            WriteNodeRaw(filename, leftNodeIndex, leftBuf);
            delete[] leftBuf;

            // Write Right Node (3)
            int *rightBuf = new int[ROW_SIZE];
            for(int k=0; k<ROW_SIZE; k++) rightBuf[k] = -1;
            rightBuf[0] = 0; // Leaf
            idx = 1;
            for(size_t i=mid; i<entries.size(); i++) {
                rightBuf[idx++] = entries[i].key;
                rightBuf[idx++] = entries[i].reference;
            }
            WriteNodeRaw(filename, rightNodeIndex, rightBuf);
            delete[] rightBuf;

            // Update Root (1) -> Points to 2 then 3
            int *rootBuf = new int[ROW_SIZE];
            for(int k=0; k<ROW_SIZE; k++) rootBuf[k] = -1;
            rootBuf[0] = 1; // Internal
            rootBuf[1] = maxLeft; rootBuf[2] = leftNodeIndex;
            rootBuf[3] = maxRight; rootBuf[4] = rightNodeIndex;
            WriteNodeRaw(filename, 1, rootBuf);
            delete[] rootBuf;

            delete[] buffer;
            return 1;
        }

        // --- Normal Split (Not Root) ---
        int rightNodeIndex = GetFreeNode(filename);

        int *rightBuf = new int[ROW_SIZE];
        for (int k = 0; k < ROW_SIZE; k++) rightBuf[k] = -1;
        rightBuf[0] = 0; // Leaf

        int idx = 1;
        for (size_t i = mid; i < entries.size(); i++) {
            rightBuf[idx++] = entries[i].key;
            rightBuf[idx++] = entries[i].reference;
        }
        WriteNodeRaw(filename, rightNodeIndex, rightBuf);

        // Update Current (Left)
        for (int i = 1; i < ROW_SIZE; i++) buffer[i] = -1;
        buffer[0] = 0; // Leaf
        idx = 1;
        for (int i = 0; i < mid; i++) {
            buffer[idx++] = entries[i].key;
            buffer[idx++] = entries[i].reference;
        }
        WriteNodeRaw(filename, currentNode, buffer);

        // Propagate
        path.pop_back();
        int parentRRN = path.back();

        // Update key for Left Child in Parent
        int *parentBuf = new int[ROW_SIZE];
        ReadNodeRaw(filename, parentRRN, parentBuf);
        for(int i=1; i<ROW_SIZE; i+=2) {
            if(parentBuf[i+1] == currentNode) {
                parentBuf[i] = maxLeft;
                WriteNodeRaw(filename, parentRRN, parentBuf);
                break;
            }
        }
        delete[] parentBuf;

        insertIntoInternal(filename, parentRRN, maxRight, rightNodeIndex, path);
        delete[] rightBuf;
    }
    delete[] buffer;
    return currentNode;
}