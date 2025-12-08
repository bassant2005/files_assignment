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
const int rowSize = 12;

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

/// ----------------- Find leaf -----------------
int findLeafForKey(fstream &file, int key, vector<int> &path) {
    int current = 1; // root RRN
    path.clear();
    path.push_back(current);

    while (true) {
        BTreeNode node = readNode(file, current);

        if (node.status == EMPTY_NODE || node.status == LEAF_NODE)
            return current;

        // find first key greater than target
        int i = 0;
        while (i < M && node.keys[i] != -1 && key >= node.keys[i]) i++;

        int child = -1;
        if (i < M && node.refs[i] != -1)
            child = node.refs[i];
        else {
            // pick last valid reference in node
            for (int j = M-1; j >= 0; j--) {
                if (node.refs[j] != -1) {
                    child = node.refs[j];
                    break;
                }
            }
        }

        if (child == -1) return current; // no child exists, treat current as leaf
        current = child;
        path.push_back(current);
    }
}

// helper: find maximum key in a node (rightmost non -1)
int maxKeyInNode(const BTreeNode &n){
    for(int i=M-1;i>=0;i--) if(n.keys[i]!=-1) return n.keys[i];
    return -1;
}
/// ----------------- Leaf Insert -----------------
int InsertNewRecordAtIndex(char* filename,int RecordID,int Reference){
    fstream file(filename, ios::in|ios::out|ios::binary);
    if(!file) return -1;
    // 1. Read root (RRN 1)
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

// 2. Find leaf (store path)
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

/// ----------------- Leaf Delete -----------------
//void DeleteRecordFromIndex(char* filename, int RecordID){
//    fstream file(filename, ios::in|ios::out|ios::binary);
//    if(!file) return;
//
//    int current=1;
//    bool found=false; int leafRRN=-1; BTreeNode node;
//
//    while(current!=-1){
//        node=readNode(file,current);
//        if(node.status==EMPTY_NODE) break;
//        if(node.status==LEAF_NODE){
//            for(int i=0;i<M;i++) if(node.keys[i]==RecordID){found=true; leafRRN=current; break;}
//            break;
//        } else {
//            int i=0;
//            while(i<M && node.keys[i]!=-1 && RecordID>=node.keys[i]) i++;
//            int child=-1;
//            if(i<M && node.refs[i]!=-1) child=node.refs[i];
//            else for(int j=M-1;j>=0;j--) if(node.refs[j]!=-1){child=node.refs[j]; break;}
//            if(child==-1) break;
//            current=child;
//        }
//    }
//
//    if(!found){file.close(); return;}
//    BTreeNode leaf=readNode(file,leafRRN);
//    int pos=-1; for(int i=0;i<M;i++) if(leaf.keys[i]==RecordID){pos=i; break;}
//    for(int i=pos;i<M-1;i++){leaf.keys[i]=leaf.keys[i+1]; leaf.refs[i]=leaf.refs[i+1];}
//    leaf.keys[M-1]=-1; leaf.refs[M-1]=-1;
//
//    bool allEmpty=true; for(int i=0;i<M;i++) if(leaf.keys[i]!=-1){allEmpty=false; break;}
//    if(allEmpty) releaseNodeToFreeList(file, leafRRN);
//    else writeNode(file, leaf);
//
//    file.close();
//}

/// ================== TESTING MAIN ==================
int main() {
//    const char filename[] = "btree_test.bin";
//    int numberOfNodes = 10;
//
//    CreateIndexFile(filename, numberOfNodes);
//    cout << "Empty B-Tree file created.\n\n";
//
//    cout << "Initial B-Tree index file content:\n";
//    DisplayIndexFileContent(filename, numberOfNodes);
//    cout << "\n";
//
//    // Open file for reading/writing
//    fstream file(filename, ios::in | ios::out | ios::binary);
//
//    // Example insertion: Node 1
//    BTreeNode n1;
//    n1.selfRRN = 1;
//    n1.status = 0; // leaf
//    n1.keys = {3,7,10,14,24};
//    n1.refs = {12,24,48,72,60};
//    writeNode(file, n1);
//
//    cout << "After first insertion:\n";
//    DisplayIndexFileContent(filename, numberOfNodes);
//
//    file.close();
//
//    int searchKeys[] = {0,7,4567,10,14,24,5};
//    for (int key : searchKeys) {
//        int ref = SearchARecord(filename, key, numberOfNodes);
//        if (ref != -1) cout << "Record " << key << " found with reference: " << ref << "\n";
//        else cout << "Record " << key << " not found.\n";
//    }
    const char filename[] = "btree_test_H.bin";
    int numberOfNodes = 10;

    CreateIndexFile((char*)filename, numberOfNodes);
    cout<<"Empty B-Tree file created.\n\n";

    cout<<"Initial B-Tree index file content:\n";
    DisplayIndexFileContent((char*)filename);
    cout<<"\n";

    int inserts[][2]={{3,12},{7,24},{10,48},{24,60},{14,72}};
    for(auto &p: inserts){
        int res=InsertNewRecordAtIndex((char*)filename,p[0],p[1]);
        cout<<"Insert ("<<p[0]<<","<<p[1]<<") -> res = "<<res<<"\n";
    }
    cout<<"\nAfter first batch of inserts:\n";
    DisplayIndexFileContent((char*)filename);
    cout<<"\n";

    int r=InsertNewRecordAtIndex((char*)filename,19,84);
    cout<<"Insert (19,84) -> res = "<<r<<"\n\n";
    cout<<"After inserting 19,84:\n";
    DisplayIndexFileContent((char*)filename);
    cout<<"\n";
//nour part
//    int moreInserts[][2]={{30,96},{15,108},{1,120},{5,132},{2,144}};
//    for(auto &p: moreInserts){
//        int res=InsertNewRecordAtIndex((char*)filename,p[0],p[1]);
//        cout<<"Insert ("<<p[0]<<","<<p[1]<<") -> res = "<<res<<"\n";
//        if(res==-2) cout<<"  -> Non-root leaf split; internal insert required (N's task).\n";
//    }
//    cout<<"\nAfter more inserts:\n";
//    DisplayIndexFileContent((char*)filename);
//    cout<<"\n";

//    cout<<"Delete: 10\n";
//    DeleteRecordFromIndex((char*)filename,10);
//    DisplayIndexFileContent((char*)filename);
//    cout<<"\n";
//
//    cout<<"Delete: 9\n";
//    DeleteRecordFromIndex((char*)filename,9);
//    DisplayIndexFileContent((char*)filename);
//    cout<<"\n";

    return 0;
}
