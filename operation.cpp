#include "BuildABtree.cpp"
/**
 * this file is created by Habeba Hossam , id : 20230117
 * Includes free-list helpers so leaf operations work standalone
 * int InsertNewRecordAtIndex (Char* filename, int RecordID, int Reference)
 * void DeleteRecordFromIndex (Char* filename, int RecordID)
 **/
const int M = 5;
const int EMPTY_NODE = -1;
const int LEAF_NODE = 0;
const int INTERNAL_NODE = 1;

/// ----------------- Free List Helpers -----------------

// Pop first free node from free list
int allocateFreeNode(fstream &file) {
    vector<int> row0(rowSize, -1);
    file.seekg(0, ios::beg);
    file.read(reinterpret_cast<char*>(row0.data()), rowSize * sizeof(int));
    int firstFree = row0[1];
    if (firstFree == -1) return -1; // no free nodes

    vector<int> freerow(rowSize, -1);
    file.seekg(firstFree * rowSize * sizeof(int), ios::beg);
    file.read(reinterpret_cast<char*>(freerow.data()), rowSize * sizeof(int));
    int nextFree = freerow[1];

    row0[1] = nextFree;
    file.seekp(0, ios::beg);
    file.write(reinterpret_cast<char*>(row0.data()), rowSize * sizeof(int));
    file.flush();

    return firstFree;
}

// Return RRN to free list
void releaseNodeToFreeList(fstream &file, int rrn) {
    vector<int> row0(rowSize, -1);
    file.seekg(0, ios::beg);
    file.read(reinterpret_cast<char*>(row0.data()), rowSize * sizeof(int));
    int firstFree = row0[1];

    vector<int> row(rowSize, -1);
    row[0] = EMPTY_NODE;
    row[1] = firstFree;
    file.seekp(rrn * rowSize * sizeof(int), ios::beg);
    file.write(reinterpret_cast<char*>(row.data()), rowSize * sizeof(int));
    file.flush();

    row0[1] = rrn;
    file.seekp(0, ios::beg);
    file.write(reinterpret_cast<char*>(row0.data()), rowSize * sizeof(int));
    file.flush();
}

/// Helper: find maximum key in a node (rightmost non -1)
int maxKeyInNode(const BTreeNode &n){
    for(int i=M-1;i>=0;i--)
        if(n.keys[i]!=-1)
            return n.keys[i];
    return -1;
}

// Helper: Count how many valid keys are in a node
int countKeys(const BTreeNode& node) {
    int count = 0;
    for (int i = 0; i < M; i++) {
        if (node.keys[i] != -1) count++;
    }
    return count;
}

// Helper: Count how many valid references are in a node
int countRefs(const BTreeNode& node) {
    int count = 0;
    for (int i = 0; i < M; i++) {
        if (node.refs[i] != -1) count++;
    }
    return count;
}
int InsertNewRecordAtIndex(char* filename,int RecordID,int Reference){
    fstream file(filename, ios::in|ios::out|ios::binary);
    if(!file) return -1;
    // Read root (RRN 1)
    BTreeNode root = readNode(file,1);
    if(root.status==EMPTY_NODE){
        // Initializing the tree: remove RRN 1 from free list
        vector<int> header(rowSize, -1);
        file.seekg(0, ios::beg);
        file.read(reinterpret_cast<char*>(header.data()), rowSize*sizeof(int));
        int head = header[1]; // should be 1
        if(head == 1){
            vector<int> freeRow(rowSize, -1);
            file.seekg(1 * rowSize * sizeof(int), ios::beg);
            file.read(reinterpret_cast<char*>(freeRow.data()), rowSize*sizeof(int));
            header[1] = freeRow[1];
            file.seekp(0, ios::beg);
            file.write(reinterpret_cast<char*>(header.data()), rowSize*sizeof(int));
            file.flush();
        }
        // Initialize root as leaf
        root.selfRRN = 1;
        root.status = LEAF_NODE;
        root.keys.assign(M, -1);
        root.refs.assign(M, -1);
        root.keys[0] = RecordID;
        root.refs[0] = Reference;
        writeNode(file,root);
        file.close();
        return 1;
    }

// Find leaf (store path)
    int current = 1;
    vector<int> path;
    while(true){
        BTreeNode node = readNode(file,current);
        path.push_back(current);
        // count keys in the leaf node
        if(node.status == LEAF_NODE){
            int cnt = 0;
            while(cnt < M && node.keys[cnt] != -1) cnt++;

            // Case A: leaf has room
            if(cnt < M){
                int pos = 0;
                while(pos < cnt && node.keys[pos] < RecordID) pos++;
                for(int i = cnt; i > pos; --i){
                    node.keys[i] = node.keys[i-1];
                    node.refs[i] = node.refs[i-1];
                }
                node.keys[pos] = RecordID;
                node.refs[pos] = Reference;
                writeNode(file,node);
                file.close();
                return node.selfRRN;
            }

            // Case B: leaf full -> split
            vector<pair<int,int>> entries;
            for(int i=0;i<M;i++) entries.push_back({node.keys[i], node.refs[i]});
            entries.push_back({RecordID, Reference});
            sort(entries.begin(), entries.end(), [](const pair<int,int>& a, const pair<int,int>& b){
                return a.first < b.first;
            });

            // left 3, right 3
            int leftSize = 3;
            int rightSize = (int)entries.size() - leftSize;

            // if parent exists, normal leaf split
            if(path.size() >= 2){
                int parentRRN = path[path.size()-2];
                BTreeNode parent = readNode(file,parentRRN);

                // create left leaf (reuse current node)
                node.keys.assign(M, -1);
                node.refs.assign(M, -1);
                for(int i=0;i<leftSize;i++){
                    node.keys[i] = entries[i].first;
                    node.refs[i] = entries[i].second;
                }

                // create right leaf
                int newRRN = allocateFreeNode(file);
                if(newRRN == -1){ file.close(); return -1; }
                BTreeNode newLeaf;
                newLeaf.selfRRN = newRRN;
                newLeaf.status = LEAF_NODE;
                newLeaf.keys.assign(M, -1);
                newLeaf.refs.assign(M, -1);
                for(int i=0;i<rightSize;i++){
                    newLeaf.keys[i] = entries[leftSize + i].first;
                    newLeaf.refs[i] = entries[leftSize + i].second;
                }

                // write leaves Update file with both leaf nodes
                writeNode(file,node);
                writeNode(file,newLeaf);

                // update parent
                vector<pair<int,int>> pentries;
                for(int i=0;i<M;i++) if(parent.keys[i]!=-1) pentries.emplace_back(parent.keys[i], parent.refs[i]);
                pentries.emplace_back(maxKeyInNode(newLeaf), newLeaf.selfRRN);
                sort(pentries.begin(), pentries.end(), [](const pair<int,int>& a, const pair<int,int>& b){ return a.first < b.first; });
                parent.keys.assign(M, -1);
                parent.refs.assign(M, -1);
                for(size_t i=0;i<pentries.size() && i<M;i++){
                    parent.keys[i] = pentries[i].first;
                    parent.refs[i] = pentries[i].second;
                }
                writeNode(file,parent);

                file.close();
                return newLeaf.selfRRN;
            } else {
                // ROOT WAS LEAF -> root split
                int leftRRN = allocateFreeNode(file);
                int rightRRN = allocateFreeNode(file);
                if(leftRRN==-1||rightRRN==-1){file.close();return -1;}

                BTreeNode leftLeaf;
                leftLeaf.selfRRN = leftRRN;
                leftLeaf.status = LEAF_NODE;
                leftLeaf.keys.assign(M,-1);
                leftLeaf.refs.assign(M,-1);
                for(int i=0;i<3;i++){
                    leftLeaf.keys[i] = entries[i].first;
                    leftLeaf.refs[i] = entries[i].second;
                }

                BTreeNode rightLeaf;
                rightLeaf.selfRRN = rightRRN;
                rightLeaf.status = LEAF_NODE;
                rightLeaf.keys.assign(M,-1);
                rightLeaf.refs.assign(M,-1);
                for(int i=0;i<3;i++){
                    rightLeaf.keys[i] = entries[i+3].first;
                    rightLeaf.refs[i] = entries[i+3].second;
                }

                writeNode(file,leftLeaf);
                writeNode(file,rightLeaf);

                BTreeNode newRoot;
                newRoot.selfRRN = 1;
                newRoot.status = INTERNAL_NODE;
                newRoot.keys.assign(M,-1);
                newRoot.refs.assign(M,-1);
                newRoot.keys[0] = leftLeaf.keys[2];
                newRoot.keys[1] = rightLeaf.keys[2];
                newRoot.refs[0] = leftRRN;
                newRoot.refs[1] = rightRRN;
                writeNode(file,newRoot);

                file.close();
                return 1;
            }
        }
        else {
            int next = -1;
            for(int i=0;i<M;i++){
                if(node.refs[i]!=-1){next=node.refs[i];break;}
            }
            if(next==-1){file.close();return -1;}
            current = next;
        }
    }

}
/// ----------------- Find position of child in parent -----------------
int findChildPositionInParent(const BTreeNode& parent, int childRRN) {
    for(int i = 0; i < M; i++) {
        if(parent.refs[i] == childRRN) {
            return i;
        }
    }
    return -1;
}

/// ----------------- Find leaf for key with path tracking -----------------
int findLeafForKey(fstream &file, int key, vector<int> &path, vector<int> &childIndices) {
    int current = 1; // root RRN
    path.clear();
    childIndices.clear();
    path.push_back(current);

    while (true) {
        BTreeNode node = readNode(file, current);

        if (node.status == EMPTY_NODE || node.status == LEAF_NODE)
            return current;

        // find first key greater than or equal to target
        // In B+ tree, keys are max values of subtrees, so we traverse right until key > separator
        int i = 0;
        while (i < M && node.keys[i] != -1 && key > node.keys[i]) i++;

        childIndices.push_back(i); // Store which child we took

        int child = -1;
        if (i < M && node.refs[i] != -1) {
            child = node.refs[i];
        } else {
            // pick last valid reference in node (for keys that are larger than all separators)
            for (int j = M-1; j >= 0; j--) {
                if (node.refs[j] != -1) {
                    child = node.refs[j];
                    break;
                }
            }
        }

        if (child == -1) return current;
        current = child;
        path.push_back(current);
    }
}

/// ----------------- Update parent separator keys -----------------
void updateParentSeparators(fstream& file, int leafRRN, int deletedKey,const vector<int>& path, const vector<int>& childIndices) {
    if (path.size() <= 1) return; // No parent

    // Check all ancestors to see if they contain the deleted key as separator
    for (int level = path.size() - 2; level >= 0; level--) {
        int parentRRN = path[level];
        int childIndex = childIndices[level];

        BTreeNode parent = readNode(file, parentRRN);

        // Check if parent has the deleted key as a separator
        if (childIndex > 0 && childIndex <= M) {
            // For B+ tree style, parent keys are usually max of left child
            // But in B-tree, they're separator keys
            for (int i = 0; i < M; i++) {
                if (parent.keys[i] == deletedKey) {
                    // We need to find the new max in the child
                    int childRRN = parent.refs[childIndex];
                    BTreeNode child = readNode(file, childRRN);
                    int newMax = maxKeyInNode(child);

                    if (newMax != -1) {
                        parent.keys[i] = newMax;
                        writeNode(file, parent);
                    }
                    break;
                }
            }
        }
    }
}

/// ----------------- Borrow from left sibling -----------------
bool borrowFromLeftSibling(fstream& file, BTreeNode& node, BTreeNode& parent, int nodePosInParent, BTreeNode& leftSibling) {
    int nodeKeyCount = countKeys(node);
    int leftKeyCount = countKeys(leftSibling);

    if(leftKeyCount <= 2) return false; // Left sibling has minimum keys

    // Get last key from left sibling
    int borrowedKey = leftSibling.keys[leftKeyCount-1];
    int borrowedRef = leftSibling.refs[leftKeyCount-1];

    // Shift node's keys right
    for(int i = nodeKeyCount; i > 0; i--) {
        node.keys[i] = node.keys[i-1];
        node.refs[i] = node.refs[i-1];
    }

    // Insert borrowed key at beginning
    node.keys[0] = borrowedKey;
    node.refs[0] = borrowedRef;

    // Remove from left sibling
    leftSibling.keys[leftKeyCount-1] = -1;
    leftSibling.refs[leftKeyCount-1] = -1;

    // Update parent separator (key between left sibling and node)
    if(nodePosInParent > 0) { // as the first node has no left sibbling
        parent.keys[nodePosInParent-1] = maxKeyInNode(leftSibling);
    }

    return true;
}

/// ----------------- Borrow from right sibling -----------------
bool borrowFromRightSibling(fstream& file, BTreeNode& node, BTreeNode& parent, int nodePosInParent, BTreeNode& rightSibling) {
    int nodeKeyCount = countKeys(node);
    int rightKeyCount = countKeys(rightSibling);

    if(rightKeyCount <= 2) return false; // Right sibling has minimum keys

    // Get first key from right sibling
    int borrowedKey = rightSibling.keys[0];
    int borrowedRef = rightSibling.refs[0];

    // Insert borrowed key at end of node
    node.keys[nodeKeyCount] = borrowedKey;
    node.refs[nodeKeyCount] = borrowedRef;

    // Shift right sibling's keys left
    for(int i = 0; i < rightKeyCount-1; i++) {
        rightSibling.keys[i] = rightSibling.keys[i+1];
        rightSibling.refs[i] = rightSibling.refs[i+1];
    }
    rightSibling.keys[rightKeyCount-1] = -1;
    rightSibling.refs[rightKeyCount-1] = -1;

    // Update parent separator
    if(nodePosInParent < M-1) {
        parent.keys[nodePosInParent] = maxKeyInNode(node);
    }

    return true;
}

/// ----------------- Merge with left sibling -----------------
void mergeWithLeftSibling(fstream& file, BTreeNode& node, BTreeNode& parent, int nodePosInParent, BTreeNode& leftSibling) {
    int leftKeyCount = countKeys(leftSibling);
    int nodeKeyCount = countKeys(node);

    // Move all keys from node to left sibling
    for(int i = 0; i < nodeKeyCount; i++) {
        leftSibling.keys[leftKeyCount + i] = node.keys[i];
        leftSibling.refs[leftKeyCount + i] = node.refs[i];
    }
    // In B+ tree, parent.keys[i] is the max key in subtree at parent.refs[i]
    // After merging node into leftSibling:
    //  Update keys[nodePosInParent-1] to be max of merged leftSibling
    //  Remove keys[nodePosInParent] and shift keys left
    //  Remove refs[nodePosInParent] and shift refs left

    // Update the key for leftSibling (now contains merged data)
    if(nodePosInParent > 0) {
        parent.keys[nodePosInParent-1] = maxKeyInNode(leftSibling);
    }

    // Remove key and reference for the merged node, then shift
    for(int i = nodePosInParent; i < M-1; i++) {
        parent.keys[i] = parent.keys[i+1];
    }
    parent.keys[M-1] = -1;

    // Remove reference to merged node and shift
    for(int i = nodePosInParent; i < M-1; i++) {
        parent.refs[i] = parent.refs[i+1];
    }
    parent.refs[M-1] = -1;

    // Free the node
    releaseNodeToFreeList(file, node.selfRRN);
}

/// ----------------- Merge with right sibling -----------------
void mergeWithRightSibling(fstream& file, BTreeNode& node, BTreeNode& parent, int nodePosInParent, BTreeNode& rightSibling) {
    int nodeKeyCount = countKeys(node);
    int rightKeyCount = countKeys(rightSibling);

    // Move all keys from right sibling to node
    for(int i = 0; i < rightKeyCount; i++) {
        node.keys[nodeKeyCount + i] = rightSibling.keys[i];
        node.refs[nodeKeyCount + i] = rightSibling.refs[i];
    }
    // In B+ tree, parent.keys[i] is the max key in subtree at parent.refs[i]
    // After merging rightSibling into node:
    // Update keys[nodePosInParent] to be max of merged node
    // Remove keys[nodePosInParent+1] (the key for rightSibling) and shift keys left
    // Remove refs[nodePosInParent+1] (the ref for rightSibling) and shift refs left

    // Update the key for node (now contains merged data)
    parent.keys[nodePosInParent] = maxKeyInNode(node);
    //cout << "Updated parent key at index " << nodePosInParent << " to " << maxKeyInNode(node) << " (max of merged node)" << endl;

    // Remove key and reference for the right sibling, then shift
    for(int i = nodePosInParent+1; i < M-1; i++) {
        parent.keys[i] = parent.keys[i+1];
    }
    parent.keys[M-1] = -1;

    // Remove reference to right sibling and shift
    for(int i = nodePosInParent+1; i < M-1; i++) {
        parent.refs[i] = parent.refs[i+1];
    }
    parent.refs[M-1] = -1;

    // Free the right sibling
    releaseNodeToFreeList(file, rightSibling.selfRRN);
}

/// ----------------- Fix underflow -----------------
void fixUnderflow(fstream& file, int nodeRRN, vector<int>& path, vector<int>& childIndices) {
    if(nodeRRN == 1) {
        // Root can have any number of keys, but if it has only 1 child, promote that child
        BTreeNode root = readNode(file, 1);
        int childCount = countRefs(root);
        if(childCount == 1) {
            // Root has only 1 child - promote child to root
            int childRRN = -1;
            for(int i = 0; i < M; i++) {
                if(root.refs[i] != -1) {
                    childRRN = root.refs[i];
                    break;
                }
            }
            if(childRRN != -1) {
                BTreeNode child = readNode(file, childRRN);
                // Make child the new root
                child.selfRRN = 1;
                writeNode(file, child);
                // Free the old child node (it's now at position 1)
                releaseNodeToFreeList(file, childRRN);
            }
        }
        return;
    }

    BTreeNode node = readNode(file, nodeRRN);
    int minKeys = 2; // ceil(M/2)-1 for M=5

    if(countKeys(node) >= minKeys) return;

    // Find parent and our position in parent
    int parentRRN = path[path.size()-2];
    BTreeNode parent = readNode(file, parentRRN);

    // Find which child we are
    int nodePosInParent = -1;
    for(int i = 0; i < M; i++) {
        if(parent.refs[i] == nodeRRN) {
            nodePosInParent = i;
            break;
        }
    }

    if(nodePosInParent == -1) {
        // Try using childIndices if available
        if(childIndices.size() >= path.size() - 1) {
            nodePosInParent = childIndices[path.size()-2];
        }
    }

    if(nodePosInParent == -1) return;

    // Try to borrow from left sibling
    // Try to borrow from left sibling
    if (nodePosInParent > 0 && parent.refs[nodePosInParent - 1] != -1) {
        int leftSiblingRRN = parent.refs[nodePosInParent - 1];
        BTreeNode leftSibling = readNode(file, leftSiblingRRN);

        // Capture old max key of left sibling BEFORE borrowing
        int oldKey = maxKeyInNode(leftSibling);

        // Try to borrow
        if (borrowFromLeftSibling(file, node, parent, nodePosInParent, leftSibling)) {
            // Write updated nodes
            writeNode(file, node);
            writeNode(file, leftSibling);

            // Update parent separator keys along the path
            updateParentSeparators(file, leftSibling.selfRRN, oldKey, path, childIndices);

            return; // Borrowing successful, done
        }
    }

    // Try to borrow from right sibling
    if (nodePosInParent < M - 1 && parent.refs[nodePosInParent + 1] != -1) {
        int rightSiblingRRN = parent.refs[nodePosInParent + 1];
        BTreeNode rightSibling = readNode(file, rightSiblingRRN);

        // Capture old max key of the current node BEFORE borrowing
        int oldKey = maxKeyInNode(node);

        // Try to borrow
        if (borrowFromRightSibling(file, node, parent, nodePosInParent, rightSibling)) {
            // Write updated nodes
            writeNode(file, node);
            writeNode(file, rightSibling);

            // Update parent separator keys along the path
            updateParentSeparators(file, node.selfRRN, oldKey, path, childIndices);

            return; // Borrowing successful, done
        }
    }

    // Need to merge
    if(nodePosInParent > 0) {
        // Merge with left sibling
        int leftSiblingRRN = parent.refs[nodePosInParent-1];
        BTreeNode leftSibling = readNode(file, leftSiblingRRN);


        mergeWithLeftSibling(file, node, parent, nodePosInParent, leftSibling);

        // After merge, the merged data is in leftSibling
        // The parent reference at nodePosInParent should point to leftSibling
        // But mergeWithLeftSibling already shifts refs, so we need to update the correct position
        // Since we merged node into leftSibling, parent.refs[nodePosInParent-1] already points to leftSibling
        // We just need to make sure the parent is written correctly
        writeNode(file, leftSibling);
        writeNode(file, parent);

        // Check if parent is now underfull or if root has only 1 child
        if(parentRRN == 1) {
            // If root has only 1 child, promote that child to root
            int childCount = countRefs(parent);
            if(childCount == 1) {
                // Find the single child
                int childRRN = -1;
                for(int i = 0; i < M; i++) {
                    if(parent.refs[i] != -1) {
                        childRRN = parent.refs[i];
                        break;
                    }
                }
                if(childRRN != -1) {
                    BTreeNode child = readNode(file, childRRN);
                    // Make child the new root
                    child.selfRRN = 1;
                    writeNode(file, child);
                    // Free the old child node (it's now at position 1)
                    releaseNodeToFreeList(file, childRRN);
                }
            }
        } else if(countKeys(parent) < minKeys) {
            // Recursively fix parent (not root)
            path.pop_back(); // Remove current node from path
            childIndices.pop_back(); // Remove current index
            fixUnderflow(file, parentRRN, path, childIndices);
        }
    } else if(nodePosInParent < M-1 && parent.refs[nodePosInParent+1] != -1) {
        // Merge with right sibling
        int rightSiblingRRN = parent.refs[nodePosInParent+1];
        BTreeNode rightSibling = readNode(file, rightSiblingRRN);


        mergeWithRightSibling(file, node, parent, nodePosInParent, rightSibling);

        writeNode(file, node);
        writeNode(file, parent);

        // Check if parent is now underfull or if root has only 1 child
        if(parentRRN == 1) {
            // If root has only 1 child, promote that child to root
            int childCount = countRefs(parent);
            if(childCount == 1) {
                // Find the single child
                int childRRN = -1;
                for(int i = 0; i < M; i++) {
                    if(parent.refs[i] != -1) {
                        childRRN = parent.refs[i];
                        break;
                    }
                }
                if(childRRN != -1) {
                    BTreeNode child = readNode(file, childRRN);
                    // Make child the new root
                    child.selfRRN = 1;
                    writeNode(file, child);
                    // Free the old child node (it's now at position 1)
                    releaseNodeToFreeList(file, childRRN);
                }
            }
        } else if(countKeys(parent) < minKeys) {
            // Recursively fix parent (not root)
            path.pop_back(); // Remove current node from path
            childIndices.pop_back(); // Remove current index
            fixUnderflow(file, parentRRN, path, childIndices);
        }
    }
}

/// ----------------- Complete DeleteRecordFromIndex Function -----------------
void DeleteRecordFromIndex(char* filename, int RecordID) {
    fstream file(filename, ios::in|ios::out|ios::binary);
    if(!file) {
        cout << "Cannot open file.\n";
        return;
    }

    // Phase 1: Find the record with path and child indices tracking
    vector<int> path;
    vector<int> childIndices;
    int current = 1;
    int leafRRN = -1;
    int keyPos = -1;
    BTreeNode leaf;

    // Find leaf containing the key
    leafRRN = findLeafForKey(file, RecordID, path, childIndices);

    if(leafRRN == -1) {
        cout << "Record " << RecordID << " not found.\n";
        file.close();
        return;
    }

    // Read the leaf
    leaf = readNode(file, leafRRN);

    // Find the key position in the leaf
    keyPos = -1;
    for(int i = 0; i < M; i++) {
        if(leaf.keys[i] == RecordID) {
            keyPos = i;
            break;
        }
    }

    if(keyPos == -1) {
        cout << "Record " << RecordID << " not found in leaf.\n";
        file.close();
        return;
    }

    // Phase 2: Delete from leaf
    int oldMax = maxKeyInNode(leaf); // Store max before deletion

    for(int i = keyPos; i < M-1; i++) {
        leaf.keys[i] = leaf.keys[i+1];
        leaf.refs[i] = leaf.refs[i+1];
    }
    leaf.keys[M-1] = -1;
    leaf.refs[M-1] = -1;

    writeNode(file, leaf);

    // Check if we deleted the max key
    int newMax = maxKeyInNode(leaf);

    // Phase 3: Update parent separator keys if we deleted the max
    if(oldMax == RecordID && path.size() > 1) {
        updateParentSeparators(file, leafRRN, RecordID, path, childIndices);
    }

    // Phase 4: Check for underflow and fix it
    int minKeys = 2; // ceil(M/2)-1 for M=5

    if(countKeys(leaf) < minKeys && leafRRN != 1) {
        // Fix underflow
        fixUnderflow(file, leafRRN, path, childIndices);

        // After fixing underflow, check if node still exists (might have been merged)
        BTreeNode checkNode = readNode(file, leafRRN);
        int actualLeafRRN = leafRRN;

        // If node was merged and freed, find which node now contains the data
        if(checkNode.status == EMPTY_NODE && path.size() > 1) {
            int parentRRN = path[path.size()-2];
            BTreeNode parent = readNode(file, parentRRN);

            // Find the node that was merged with (should be a sibling)
            // If merged with left, it's at nodePosInParent-1
            // If merged with right, original node should still exist (merge right puts data in original)
            int nodePosInParent = -1;
            for(int i = 0; i < M; i++) {
                if(parent.refs[i] == leafRRN) {
                    nodePosInParent = i;
                    break;
                }
            }

            // If node was merged with left sibling, data is in left sibling
            if(nodePosInParent > 0 && parent.refs[nodePosInParent-1] != -1) {
                BTreeNode leftSib = readNode(file, parent.refs[nodePosInParent-1]);
                if(leftSib.status == LEAF_NODE) {
                    actualLeafRRN = parent.refs[nodePosInParent-1];
                }
            }
        }

        // Update parent separator if needed
        if(path.size() > 1) {
            int parentRRN = path[path.size()-2];
            BTreeNode parent = readNode(file, parentRRN);

            // Find which child position contains actualLeafRRN
            int nodePosInParent = -1;
            for(int i = 0; i < M; i++) {
                if(parent.refs[i] == actualLeafRRN) {
                    nodePosInParent = i;
                    break;
                }
            }

            if(nodePosInParent != -1) {
                // Re-read leaf after fixUnderflow
                BTreeNode actualLeaf = readNode(file, actualLeafRRN);
                int finalMax = maxKeyInNode(actualLeaf);

                // Update parent key for this node (parent.keys[i] is max of subtree at parent.refs[i])
                // We should update parent.keys[nodePosInParent], not nodePosInParent-1
                if(parent.keys[nodePosInParent] != finalMax && finalMax != -1) {
                    parent.keys[nodePosInParent] = finalMax;
                    writeNode(file, parent);
                }

                // Also update the key for left sibling if it exists (to ensure consistency)
                if(nodePosInParent > 0 && parent.refs[nodePosInParent-1] != -1) {
                    BTreeNode leftSib = readNode(file, parent.refs[nodePosInParent-1]);
                    int leftMax = maxKeyInNode(leftSib);
                    if(parent.keys[nodePosInParent-1] != leftMax && leftMax != -1) {
                        parent.keys[nodePosInParent-1] = leftMax;
                        writeNode(file, parent);
                    }
                }
            }
        }
    }

    file.close();
    cout << "Deletion process completed.\n";
}
/// ================== TESTING MAIN ==================
int main() {
    const char filename[] = "btree_test_H.bin";
    int numberOfNodes = 10;

    // Create initial file
    CreateIndexFile((char*)filename, numberOfNodes);
    cout<<"Empty B-Tree file created.\n\n";

    cout<<"Initial B-Tree index file content:\n";
    DisplayIndexFileContent((char*)filename);
    cout<<"\n";

    // Insert test data
    int inserts[][2]={{3,12},{7,24},{10,48},{24,60},{14,72}};
    for(auto &p: inserts){
        int res=InsertNewRecordAtIndex((char*)filename,p[0],p[1]);
        cout<<"Insert ("<<p[0]<<","<<p[1]<<") -> res = "<<res<<"\n";
    }
    cout<<"\nAfter first batch of inserts:\n";
    DisplayIndexFileContent((char*)filename);
    cout<<"\n";

    // Insert more to cause splitting
    int r=InsertNewRecordAtIndex((char*)filename,19,84);
    cout<<"Insert (19,84) -> res = "<<r<<"\n\n";
    cout<<"After inserting 19,84:\n";
    DisplayIndexFileContent((char*)filename);
    cout<<"\n";

    // Test search
    cout<<"Search tests:\n";
    cout<<"Search 10 -> "<<SearchARecord((char*)filename,10)<<"\n";
    cout<<"Search 24 -> "<<SearchARecord((char*)filename,24)<<"\n";
    cout<<"Search 99 -> "<<SearchARecord((char*)filename,99)<<"\n\n";

    // Test deletion - deleting max key (24)
    cout<<"Delete: 24 (this is a max key in its node)\n";
    DeleteRecordFromIndex((char*)filename,24);
    cout<<"\nAfter deleting 24:\n";
    DisplayIndexFileContent((char*)filename);
    cout<<"\n";

    // Test deleting another max key
    cout<<"Delete: 19\n";
    DeleteRecordFromIndex((char*)filename,19);
    cout<<"\nAfter deleting 19:\n";
    DisplayIndexFileContent((char*)filename);
    cout<<"\n";

    // Test deleting non-max key
    cout<<"Delete: 7\n";
    DeleteRecordFromIndex((char*)filename,7);
    cout<<"\nAfter deleting 7:\n";
    DisplayIndexFileContent((char*)filename);
    cout<<"\n";

    return 0;
}