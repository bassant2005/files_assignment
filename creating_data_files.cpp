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
#include <unordered_map>
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
        strcpy(entry.recordID, line.substr(0, pos).c_str());
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
    long long currentOffset = 0;

    while (getline(in, line)) {
        if (line.empty()) continue;
        bool deleted = (line[0] == '*');
        string cleanLine = deleted ? line.substr(1) : line;
        auto fields = split(cleanLine);
        if (fields.size() <= idFieldIndex) continue;

        PrimaryIndex entry;
        string id = fields[idFieldIndex];
        if (deleted) id = "*" + id;

        strcpy(entry.recordID, id.c_str());
        entry.offset = currentOffset;
        entry.recordLength = line.length() + 1;
        currentOffset += entry.recordLength;

        primaryIndex.push_back(entry);
    }

    in.close();

    // Sort primary index while IGNORING leading '*' in the ID
    sort(primaryIndex.begin(), primaryIndex.end(),
         [](const PrimaryIndex &a, const PrimaryIndex &b) {
             string A = (a.recordID[0] == '*') ? a.recordID + 1 : a.recordID;
             string B = (b.recordID[0] == '*') ? b.recordID + 1 : b.recordID;

             if (A == B) return a.recordID[0] != '*';
             return A < B;
         }
    );

    writePrimaryIndex(primaryIndex, indexFile);
    return primaryIndex;
}

// Build the secondary index: key = DoctorID, linkedID = AppointmentID
// Handles deletion markers '*' for each field independently
vector<SecondaryIndex> buildSecondaryIndex(const string &appointmentsFile,
                                           const string &doctorsFile,
                                           const string &indexFile,
                                           int keyFieldIndex,      // DoctorID in appointment file
                                           int linkedFieldIndex) { // AppointmentID in appointment file
    ifstream appIn(appointmentsFile);
    if (!appIn.is_open()) {
        cout << "Cannot open " << appointmentsFile << "\n";
        return {};
    }

    // Step 1: Read doctor deletion info into a map
    ifstream docIn(doctorsFile);
    unordered_map<string, bool> doctorDeleted; // DoctorID -> deleted?
    string line;
    while (getline(docIn, line)) {
        if (line.empty()) continue;
        bool deleted = line[0] == '*';
        string cleanLine = deleted ? line.substr(1) : line;
        auto fields = split(cleanLine);
        if (fields.size() < 4) continue; // DoctorID is field 3
        string doctorID = fields[3];
        doctorDeleted[doctorID] = deleted;
    }
    docIn.close();

    // Step 2: Build secondary index from appointments
    vector<SecondaryIndex> secondaryIndex;
    while (getline(appIn, line)) {
        if (line.empty()) continue;
        bool appDeleted = line[0] == '*';
        string cleanLine = appDeleted ? line.substr(1) : line;
        auto fields = split(cleanLine);
        if (fields.size() <= max(keyFieldIndex, linkedFieldIndex)) continue;

        string doctorID = fields[keyFieldIndex];
        string appointmentID = fields[linkedFieldIndex];

        if (doctorDeleted.count(doctorID) && doctorDeleted[doctorID])
            doctorID = "*" + doctorID;
        if (appDeleted)
            appointmentID = "*" + appointmentID;

        SecondaryIndex entry;
        strcpy(entry.keyValue, doctorID.c_str());
        strcpy(entry.linkedID, appointmentID.c_str());
        secondaryIndex.push_back(entry);
    }
    appIn.close();

    // Step 3: Sort secondary index ignoring '*'
    sort(secondaryIndex.begin(), secondaryIndex.end(),
         [](const SecondaryIndex &a, const SecondaryIndex &b) {
             string keyA = (a.keyValue[0] == '*') ? a.keyValue + 1 : a.keyValue;
             string keyB = (b.keyValue[0] == '*') ? b.keyValue + 1 : b.keyValue;
             if (keyA != keyB) return keyA < keyB;

             string idA = (a.linkedID[0] == '*') ? a.linkedID + 1 : a.linkedID;
             string idB = (b.linkedID[0] == '*') ? b.linkedID + 1 : b.linkedID;
             if (idA != idB) return idA < idB;

             if (a.keyValue[0] != b.keyValue[0]) return a.keyValue[0] != '*';
             if (a.linkedID[0] != b.linkedID[0]) return a.linkedID[0] != '*';
             return false;
         }
    );

    writeSecondaryIndex(secondaryIndex, indexFile);
    return secondaryIndex;
}

/**
 * REBUILD ALL THE INDEXES WHEN
 * NEEDED AFTER ANY UPDATES IN THE DATA FILES
 */
void Build_indexes(){
    auto doctorPrimary = buildPrimaryIndexLength(doctorDataFile, doctorPrimaryIndexFile, 3);
    auto doctorSecondary = buildSecondaryIndex(doctorDataFile,doctorDataFile, doctorSecondaryIndexFile, 1, 3);
    auto appointmentPrimary = buildPrimaryIndexLength(appointmentDataFile, appointmentPrimaryIndexFile, 1);
    auto appointmentSecondary = buildSecondaryIndex(appointmentDataFile,
                                                    doctorDataFile,appointmentSecondaryIndexFile, 2, 1);
}

// int main() {
//     cout << "=== Building indexes for Doctors.txt ===\n";
//     auto doctorPrimary = buildPrimaryIndexLength(doctorDataFile, doctorPrimaryIndexFile, 3); // DoctorID
//     auto doctorSecondary = buildSecondaryIndex(doctorDataFile,doctorDataFile, doctorSecondaryIndexFile, 1, 3); // Name -> ID
//
//     cout << "Doctor Primary Index (ID | Offset):\n";
//     for (auto &d : doctorPrimary)
//         cout << d.recordID << " | " << d.offset<< "\n";
//
//     cout << "\nDoctor Secondary Index (Name -> ID):\n";
//     for (auto &d : doctorSecondary)
//         cout << d.keyValue << " | " << d.linkedID << "\n";
//
//     cout << "\n=== Building indexes for Appointments.txt ===\n";
//     auto appointmentPrimary = buildPrimaryIndexLength(appointmentDataFile, appointmentPrimaryIndexFile, 1); // AppointmentID
//     auto appointmentSecondary = buildSecondaryIndex(appointmentDataFile, doctorDataFile, appointmentSecondaryIndexFile, 2, 1); // DoctorID -> AppointmentID
//
//     cout << "Appointment Primary Index (ID | Offset):\n";
//     for (auto &a : appointmentPrimary)
//         cout << a.recordID << " | " << a.offset << "\n";
//
//     cout << "\nAppointment Secondary Index (DoctorID -> AppointmentID):\n";
//     for (auto &a : appointmentSecondary)
//         cout << a.keyValue << " | " << a.linkedID << "\n";
//
//     // Test reading back from indexes files
//     cout << "\n=== Read indexes ===\n";
//     auto doctorPrimaryRead = readPrimaryIndex(doctorPrimaryIndexFile);
//     cout << "\nRead back Doctor Primary Index:\n";
//     for (auto &d : doctorPrimaryRead)
//         cout << d.recordID << " | " << d.offset << "\n";
//
//     auto doctorSecondaryRead = readSecondaryIndex(doctorSecondaryIndexFile);
//     cout << "\nRead back Doctor Secondary Index:\n";
//     for (auto &d : doctorSecondaryRead)
//         cout << d.keyValue << " | " << d.linkedID << "\n";
// }