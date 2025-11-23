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
    int recordLength;      // record position (starts at 1)

    bool operator<(const PrimaryIndex &other) const {
        return strcmp(recordID, other.recordID) < 0;
    }
};

// Each record in the secondary index stores (Key, ID)
// e.g., (DoctorName, DoctorID)
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
    for (auto &entry : indexList)
        out << entry.recordID << "|" << entry.recordLength << "\n";
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

    if (!in.is_open()) {
        cout << "DEBUG: Could not open " << fileName << " for reading\n";
        return indexList;
    }

    while (getline(in, line)) {
        if (line.empty()) continue;

        size_t pos = line.find('|');
        if (pos == string::npos) continue;

        PrimaryIndex entry;
        string id = line.substr(0, pos);
        string rrnStr = line.substr(pos + 1);

        strcpy(entry.recordID, id.c_str());
        entry.recordLength = stoi(rrnStr);
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
vector<PrimaryIndex> buildPrimaryIndexLength(const string &dataFile, const string &indexFile, int idFieldIndex) {
    ifstream in(dataFile);
    vector<PrimaryIndex> primaryIndex;
    string line;

    while (getline(in, line)) {
        if (line.empty()) continue;

        auto fields = split(line);
        if (fields.size() <= idFieldIndex) continue;

        PrimaryIndex entry;
        strcpy(entry.recordID, fields[idFieldIndex].c_str());

        // Directly take length from the first column
        entry.recordLength = stoi(fields[0]);

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

int main() {
    cout << "=== Building indexes for Doctors.txt ===\n";
    auto doctorPrimary = buildPrimaryIndexLength(doctorDataFile, doctorPrimaryIndexFile, 3); // DoctorID
    auto doctorSecondary = buildSecondaryIndex(doctorDataFile, doctorSecondaryIndexFile, 1, 3); // Name -> ID

    cout << "Doctor Primary Index (ID | RecordLength):\n";
    for (auto &d : doctorPrimary)
        cout << d.recordID << " | " << d.recordLength << "\n";

    cout << "\nDoctor Secondary Index (Name -> ID):\n";
    for (auto &d : doctorSecondary)
        cout << d.keyValue << " | " << d.linkedID << "\n";

    cout << "\n=== Building indexes for Appointments.txt ===\n";
    auto appointmentPrimary = buildPrimaryIndexLength(appointmentDataFile, appointmentPrimaryIndexFile, 1); // AppointmentID
    auto appointmentSecondary = buildSecondaryIndex(appointmentDataFile, appointmentSecondaryIndexFile, 2, 1); // DoctorID -> AppointmentID

    cout << "Appointment Primary Index (ID | RecordLength):\n";
    for (auto &a : appointmentPrimary)
        cout << a.recordID << " | " << a.recordLength << "\n";

    cout << "\nAppointment Secondary Index (DoctorID -> AppointmentID):\n";
    for (auto &a : appointmentSecondary)
        cout << a.keyValue << " | " << a.linkedID << "\n";

    // Test reading back from index files
    auto doctorPrimaryRead = readPrimaryIndex(doctorPrimaryIndexFile);
    cout << "\nRead back Doctor Primary Index:\n";
    for (auto &d : doctorPrimaryRead)
        cout << d.recordID << " | " << d.recordLength << "\n";
}
