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
// Each record in the primary index stores (ID, offset)
struct PrimaryIndex {
    char recordID[20];
    int offset;
    int recordLength;

    bool operator <(const PrimaryIndex& other) const {
        return strcmp(recordID, other.recordID) < 0;
    }
};

// Each record in the secondary index stores (Key, ID)
struct SecondaryIndex {
    char keyValue[20];  // secondary key (like doctor ID)
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
    for (const auto &entry : indexList) {
        out << entry.recordID << "|" << entry.offset << "\n";
    }
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
    string line;

    while (getline(in, line)) {
        if (line.empty()) continue;

        size_t pos = line.find('|');
        if (pos == string::npos) continue;

        PrimaryIndex entry;

        // recordID
        strcpy(entry.recordID, line.substr(0, pos).c_str());

        // offset
        entry.offset = stoll(line.substr(pos + 1));

        indexList.push_back(entry);
    }

    in.close();
    return indexList;
}

// Reads all records from a secondary index file into memory
vector<SecondaryIndex> readSecondaryIndex(const string &fileName) {
    vector<SecondaryIndex> indexList;
    ifstream in(fileName);
    string line;

    if (!in.is_open()) {
        cout << "DEBUG: Could not open " << fileName << " for reading\n";
        return indexList;
    }

    while (getline(in, line)) {
        if (line.empty()) continue;

        size_t pos = line.find('|');
        if (pos == string::npos) continue;

        SecondaryIndex entry;
        string key = line.substr(0, pos);
        string id = line.substr(pos + 1);

        strcpy(entry.keyValue, key.c_str());
        strcpy(entry.linkedID, id.c_str());
        indexList.push_back(entry);
    }

    in.close();
    return indexList;
}

//// ===================== INDEX BUILDING FUNCTIONS =====================
// BUILD PRIMARY INDEX
vector<PrimaryIndex> buildPrimaryIndexLength(const string &dataFile,
                                             const string &indexFile,
                                             int idFieldIndex) {
    ifstream in(dataFile);
    vector<PrimaryIndex> primaryIndex;
    string line;

    long long currentOffset = 0;   // start at byte 0

    while (getline(in, line)) {
        if (line.empty()) continue;

        auto fields = split(line);
        if (fields.size() <= idFieldIndex) continue;

        PrimaryIndex entry;
        strcpy(entry.recordID, fields[idFieldIndex].c_str());

        // Set offset for this record
        entry.offset = currentOffset;

        // Calculate record length: line chars + newline
        entry.recordLength = line.length() + 1;

        // Update offset for next record
        currentOffset += entry.recordLength;

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

/**
 * YOU CAN USE THIS TO REBUILD ALL THE INDEXES WHEN
 * NEEDED AFTER ANY UPDATES IN THE DATA FILES
 * */

void Build_indexes(){
    auto doctorPrimary = buildPrimaryIndexLength(doctorDataFile, doctorPrimaryIndexFile, 3);
    auto doctorSecondary = buildSecondaryIndex(doctorDataFile, doctorSecondaryIndexFile, 1, 3);
    auto appointmentPrimary = buildPrimaryIndexLength(appointmentDataFile, appointmentPrimaryIndexFile, 1);
    auto appointmentSecondary = buildSecondaryIndex(appointmentDataFile, appointmentSecondaryIndexFile, 2, 1);
}

int main() {
    cout << "=== Building indexes for Doctors.txt ===\n";
    auto doctorPrimary = buildPrimaryIndexLength(doctorDataFile, doctorPrimaryIndexFile, 3); // DoctorID
    auto doctorSecondary = buildSecondaryIndex(doctorDataFile, doctorSecondaryIndexFile, 1, 3); // Name -> ID

    cout << "Doctor Primary Index (ID | Offset):\n";
    for (auto &d : doctorPrimary)
        cout << d.recordID << " | " << d.offset<< "\n";

    cout << "\nDoctor Secondary Index (Name -> ID):\n";
    for (auto &d : doctorSecondary)
        cout << d.keyValue << " | " << d.linkedID << "\n";

    cout << "\n=== Building indexes for Appointments.txt ===\n";
    auto appointmentPrimary = buildPrimaryIndexLength(appointmentDataFile, appointmentPrimaryIndexFile, 1); // AppointmentID
    auto appointmentSecondary = buildSecondaryIndex(appointmentDataFile, appointmentSecondaryIndexFile, 2, 1); // DoctorID -> AppointmentID

    cout << "Appointment Primary Index (ID | Offset):\n";
    for (auto &a : appointmentPrimary)
        cout << a.recordID << " | " << a.offset << "\n";

    cout << "\nAppointment Secondary Index (DoctorID -> AppointmentID):\n";
    for (auto &a : appointmentSecondary)
        cout << a.keyValue << " | " << a.linkedID << "\n";

    // Test reading back from indexes files
    cout << "\n=== Read indexes ===\n";
    auto doctorPrimaryRead = readPrimaryIndex(doctorPrimaryIndexFile);
    cout << "\nRead back Doctor Primary Index:\n";
    for (auto &d : doctorPrimaryRead)
        cout << d.recordID << " | " << d.offset << "\n";

    auto doctorSecondaryRead = readSecondaryIndex(doctorSecondaryIndexFile);
    cout << "\nRead back Doctor Secondary Index:\n";
    for (auto &d : doctorSecondaryRead)
        cout << d.keyValue << " | " << d.linkedID << "\n";
}