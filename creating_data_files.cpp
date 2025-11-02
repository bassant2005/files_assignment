/**
 * ===============================================================
 *  File created by: Bassant Tarek
 *  Purpose: Implements the logic for building the indexed files
 *           (both primary and secondary indexes) for the system.
 *  Student ID: 20231023
 * ===============================================================
 */

#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <algorithm>
#include <sstream>
using namespace std;

//// ===================== FILE PATHS =====================
// These are the files used for storing the main data and their index files.
const string doctorDataFile = "Doctors.txt";
const string appointmentDataFile = "Appointments.txt";
const string doctorPrimaryIndexFile = "DoctorPrimaryIndex.txt";
const string appointmentPrimaryIndexFile = "AppointmentPrimaryIndex.txt";
const string doctorSecondaryIndexFile = "DoctorSecondaryIndex.txt";
const string appointmentSecondaryIndexFile = "AppointmentSecondaryIndex.txt";

//// ===================== STRUCT DEFINITIONS =====================

// Each record in the primary index stores (ID, RRN)
// RRN = relative record number = record's order in the data file
struct PrimaryIndex {
    char recordID[10];  // e.g., DoctorID or AppointmentID
    int recordRRN;      // record position (starts at 1)

    bool operator<(const PrimaryIndex &other) const {
        return strcmp(recordID, other.recordID) < 0;
    }
};

// Each record in the secondary index stores (Key, ID)
// e.g., (DoctorName, DoctorID)
struct SecondaryIndex {
    char keyValue[20];  // secondary key (like doctor name)
    char linkedID[10];  // ID of the related record

    bool operator<(const SecondaryIndex &other) const {
        int cmp = strcmp(keyValue, other.keyValue);
        return cmp == 0 ? strcmp(linkedID, other.linkedID) < 0 : cmp < 0;
    }
};

//// ===================== HELPER FUNCTION =====================
// Splits a line of text into parts using the given delimiter ('|')
vector<string> split(const string &line, char delim = '|') {
    vector<string> parts;
    stringstream ss(line);
    string segment;

    while (getline(ss, segment, delim))
        parts.push_back(segment);

    return parts;
}

//// ===================== WRITE FUNCTIONS =====================
// Writes the primary index (ID|RRN) to a file
void writePrimaryIndex(const vector<PrimaryIndex> &indexList, const string &fileName) {
    ofstream out(fileName);
    for (auto &entry : indexList)
        out << entry.recordID << "|" << entry.recordRRN << "\n";
    out.close();
}

// Writes the secondary index (Key|ID) to a file
void writeSecondaryIndex(const vector<SecondaryIndex> &indexList, const string &fileName) {
    ofstream out(fileName);
    for (auto &entry : indexList)
        out << entry.keyValue << "|" << entry.linkedID << "\n";
    out.close();
}

//// ===================== READ FUNCTIONS =====================
// Reads all records from a primary index file into memory
vector<PrimaryIndex> readPrimaryIndex(const string &fileName) {
    vector<PrimaryIndex> indexList;
    ifstream in(fileName);
    PrimaryIndex entry;

    while (in >> entry.recordID) {
        in.ignore(1, '|');  // skip the '|'
        in >> entry.recordRRN;
        indexList.push_back(entry);
    }

    in.close();
    return indexList;
}

// Reads all records from a secondary index file into memory
vector<SecondaryIndex> readSecondaryIndex(const string &fileName) {
    vector<SecondaryIndex> indexList;
    ifstream in(fileName);
    SecondaryIndex entry;

    while (in >> entry.keyValue) {
        in.ignore(1, '|');
        in >> entry.linkedID;
        indexList.push_back(entry);
    }

    in.close();
    return indexList;
}

//// ===================== INDEX BUILDING FUNCTIONS =====================
// BUILD PRIMARY INDEX
vector<PrimaryIndex> buildPrimaryIndex(const string &dataFile, const string &indexFile, int idFieldIndex) {
    ifstream in(dataFile);
    vector<PrimaryIndex> primaryIndex;
    string line;
    int rrn = 1;

    // read each line (record) from the file
    while (getline(in, line)) {
        if (line.empty()) continue;

        // split the record by delimiter (e.g., '|')
        auto fields = split(line);
        if (fields.size() <= idFieldIndex) continue;

        PrimaryIndex entry;
        strcpy(entry.recordID, fields[idFieldIndex].c_str());
        entry.recordRRN = rrn++;

        primaryIndex.push_back(entry);
    }

    in.close();
    sort(primaryIndex.begin(), primaryIndex.end());
    writePrimaryIndex(primaryIndex, indexFile);

    return primaryIndex;
}

// BUILD SECONDARY INDEX
vector<SecondaryIndex> buildSecondaryIndex(const string &dataFile,const string &indexFile,int keyFieldIndex,int linkedFieldIndex) {
    ifstream in(dataFile);
    vector<SecondaryIndex> secondaryIndex;
    string line;

    // read each record line by line
    while (getline(in, line)) {
        if (line.empty()) continue;

        auto fields = split(line);
        if (fields.size() <= max(keyFieldIndex, linkedFieldIndex)) continue;

        SecondaryIndex entry;
        strcpy(entry.keyValue, fields[keyFieldIndex].c_str());
        strcpy(entry.linkedID, fields[linkedFieldIndex].c_str());
        secondaryIndex.push_back(entry);
    }

    in.close();
    sort(secondaryIndex.begin(), secondaryIndex.end());
    writeSecondaryIndex(secondaryIndex, indexFile);

    return secondaryIndex;
}

// Builds all index files for both doctors and appointments
void buildAllIndexes() {
    buildPrimaryIndex(doctorDataFile, doctorPrimaryIndexFile, 1);
    buildPrimaryIndex(appointmentDataFile, appointmentPrimaryIndexFile, 1);
    buildSecondaryIndex(doctorDataFile,doctorSecondaryIndexFile,2,1);
    buildSecondaryIndex(appointmentDataFile,appointmentSecondaryIndexFile,2,1);
}

//// ===================== MAIN FUNCTION =====================
int main() {
    cout << "🔄 Building all indexes...\n";
    buildAllIndexes(); // Creates all four index files
    cout << "✅ All indexes have been created and saved successfully!\n";
}
