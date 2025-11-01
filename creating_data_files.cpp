#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <algorithm>
#include <sstream>
using namespace std;

// ===================== FILE PATHS =====================
// These are the files used for storing the main data and their index files.
const string doctorDataFile = "Doctors.txt";
const string appointmentDataFile = "Appointments.txt";
const string doctorPrimaryIndexFile = "DoctorPrimaryIndex.txt";
const string appointmentPrimaryIndexFile = "AppointmentPrimaryIndex.txt";
const string doctorSecondaryIndexFile = "DoctorSecondaryIndex.txt";
const string appointmentSecondaryIndexFile = "AppointmentSecondaryIndex.txt";

// ===================== STRUCT DEFINITIONS =====================

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

// ===================== HELPER FUNCTION =====================
// Splits a line of text into parts using the given delimiter ('|')
vector<string> split(const string &line, char delim = '|') {
    vector<string> parts;
    stringstream ss(line);
    string segment;

    while (getline(ss, segment, delim))
        parts.push_back(segment);

    return parts;
}

// ===================== WRITE FUNCTIONS =====================
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

// ===================== READ FUNCTIONS =====================
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

// ===================== INDEX BUILDING FUNCTIONS =====================

// Builds the primary index for doctors
// File format: X|DoctorID|DoctorName|DoctorAddress
vector<PrimaryIndex> buildDoctorPrimaryIndex() {
    ifstream in(doctorDataFile);
    vector<PrimaryIndex> doctorPrimaryIndex;
    string line;
    int rrn = 1; // first record is RRN=1

    while (getline(in, line)) {
        if (line.empty()) continue;
        auto fields = split(line);
        if (fields.size() < 4) continue;

        PrimaryIndex entry;
        strcpy(entry.recordID, fields[1].c_str()); // DoctorID
        entry.recordRRN = rrn++;
        doctorPrimaryIndex.push_back(entry);
    }

    in.close();
    sort(doctorPrimaryIndex.begin(), doctorPrimaryIndex.end());
    writePrimaryIndex(doctorPrimaryIndex, doctorPrimaryIndexFile);

    return doctorPrimaryIndex;
}

// Builds the primary index for appointments
// File format: X|AppointmentID|DoctorID|AppointmentDate
vector<PrimaryIndex> buildAppointmentPrimaryIndex() {
    ifstream in(appointmentDataFile);
    vector<PrimaryIndex> appointmentPrimaryIndex;
    string line;
    int rrn = 1;

    while (getline(in, line)) {
        if (line.empty()) continue;
        auto fields = split(line);
        if (fields.size() < 4) continue;

        PrimaryIndex entry;
        strcpy(entry.recordID, fields[1].c_str()); // AppointmentID
        entry.recordRRN = rrn++;
        appointmentPrimaryIndex.push_back(entry);
    }

    in.close();
    sort(appointmentPrimaryIndex.begin(), appointmentPrimaryIndex.end());
    writePrimaryIndex(appointmentPrimaryIndex, appointmentPrimaryIndexFile);

    return appointmentPrimaryIndex;
}

// Builds the secondary index for doctors (by name)
// Each entry links DoctorName → DoctorID
vector<SecondaryIndex> buildDoctorSecondaryIndex() {
    ifstream in(doctorDataFile);
    vector<SecondaryIndex> doctorSecondaryIndex;
    string line;

    while (getline(in, line)) {
        if (line.empty()) continue;
        auto fields = split(line);
        if (fields.size() < 4) continue;

        SecondaryIndex entry;
        strcpy(entry.keyValue, fields[2].c_str()); // DoctorName
        strcpy(entry.linkedID, fields[1].c_str()); // DoctorID
        doctorSecondaryIndex.push_back(entry);
    }

    in.close();
    sort(doctorSecondaryIndex.begin(), doctorSecondaryIndex.end());
    writeSecondaryIndex(doctorSecondaryIndex, doctorSecondaryIndexFile);

    return doctorSecondaryIndex;
}

// Builds the secondary index for appointments (by DoctorID)
// Each entry links DoctorID → AppointmentID
vector<SecondaryIndex> buildAppointmentSecondaryIndex() {
    ifstream in(appointmentDataFile);
    vector<SecondaryIndex> appointmentSecondaryIndex;
    string line;

    while (getline(in, line)) {
        if (line.empty()) continue;
        auto fields = split(line);
        if (fields.size() < 4) continue;

        SecondaryIndex entry;
        strcpy(entry.keyValue, fields[2].c_str()); // DoctorID
        strcpy(entry.linkedID, fields[1].c_str()); // AppointmentID
        appointmentSecondaryIndex.push_back(entry);
    }

    in.close();
    sort(appointmentSecondaryIndex.begin(), appointmentSecondaryIndex.end());
    writeSecondaryIndex(appointmentSecondaryIndex, appointmentSecondaryIndexFile);

    return appointmentSecondaryIndex;
}

// Builds all index files for both doctors and appointments
void buildAllIndexes() {
    buildDoctorPrimaryIndex();
    buildAppointmentPrimaryIndex();
    buildDoctorSecondaryIndex();
    buildAppointmentSecondaryIndex();
}

//// ===================== MAIN FUNCTION =====================
//int main() {
//    cout << "🔄 Building all indexes...\n";
//
//    buildAllIndexes(); // Creates all four index files
//
//    cout << "✅ All indexes have been created and saved successfully!\n";
//}
