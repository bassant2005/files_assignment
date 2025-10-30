#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <algorithm>
using namespace std;

// ===================== FILE PATHS =====================
// File names for data and index files
const string doctorFile = "Doctors.txt";
const string appointmentFile = "Appointments.txt";
const string doctorPrimaryFile = "DoctorPrimaryIndex.txt";
const string appointmentPrimaryFile = "AppointmentPrimaryIndex.txt";
const string doctorSecondaryFile = "DoctorSecondaryIndex.txt";
const string appointmentSecondaryFile = "AppointmentSecondaryIndex.txt";

// ===================== STRUCTS =====================
// Doctor record
struct Doctor {
    char doctorID[10];
    char doctorName[20];
    char Address[30];
};

// Appointment record
struct Appointment {
    char appointmentID[10];
    char doctorID[10];
    char appointmentDate[15];
};

// Primary index: links ID to record line number (RRN)
struct PrimaryIndex {
    char ID[10];
    int RRN;
    bool operator<(const PrimaryIndex &r) const {
        return strcmp(ID, r.ID) < 0;
    }
};

// Secondary index: links a secondary key (like name) to an ID
struct SecondaryIndex {
    char key[20];
    char ID[10];
    bool operator<(const SecondaryIndex &r) const {
        int cmp = strcmp(key, r.key);
        return cmp == 0 ? strcmp(ID, r.ID) < 0 : cmp < 0;
    }
};

// ===================== WRITE FUNCTIONS =====================
// Save primary index to file
void writePrimaryIndex(const vector<PrimaryIndex> &arr, const string &fileName) {
    ofstream out(fileName);
    for (auto &p : arr)
        out << p.ID << "|" << p.RRN << "\n";
    out.close();
}

// Save secondary index to file
void writeSecondaryIndex(const vector<SecondaryIndex> &arr, const string &fileName) {
    ofstream out(fileName);
    for (auto &s : arr)
        out << s.key << "|" << s.ID << "\n";
    out.close();
}

// ===================== READ FUNCTIONS =====================
// Read primary index from file
vector<PrimaryIndex> readPrimaryIndex(const string &fileName) {
    vector<PrimaryIndex> arr;
    ifstream in(fileName);
    PrimaryIndex p;

    while (in >> p.ID) {
        in.ignore(1, '|');
        in >> p.RRN;
        arr.push_back(p);
    }

    in.close();
    return arr;
}

// Read secondary index from file
vector<SecondaryIndex> readSecondaryIndex(const string &fileName) {
    vector<SecondaryIndex> arr;
    ifstream in(fileName);
    SecondaryIndex s;

    while (in >> s.key) {
        in.ignore(1, '|');
        in >> s.ID;
        arr.push_back(s);
    }

    in.close();
    return arr;
}

// ===================== BUILD FUNCTIONS =====================

// Create primary index for doctors (by doctor ID)
vector<PrimaryIndex> buildDoctorPrimaryIndex() {
    ifstream in(doctorFile);
    vector<PrimaryIndex> index;
    string line;
    int rrn = 0;

    while (getline(in, line)) {
        if (line.empty()) continue;
        PrimaryIndex p;
        string id = line.substr(0, line.find('|'));
        strcpy(p.ID, id.c_str());
        p.RRN = rrn++;
        index.push_back(p);
    }

    in.close();
    sort(index.begin(), index.end());
    writePrimaryIndex(index, doctorPrimaryFile);
    return index;
}

// Create primary index for appointments (by appointment ID)
vector<PrimaryIndex> buildAppointmentPrimaryIndex() {
    ifstream in(appointmentFile);
    vector<PrimaryIndex> index;
    string line;
    int rrn = 0;

    while (getline(in, line)) {
        if (line.empty()) continue;
        PrimaryIndex p;
        string id = line.substr(0, line.find('|'));
        strcpy(p.ID, id.c_str());
        p.RRN = rrn++;
        index.push_back(p);
    }

    in.close();
    sort(index.begin(), index.end());
    writePrimaryIndex(index, appointmentPrimaryFile);
    return index;
}

// Create secondary index for doctors (by doctor name)
vector<SecondaryIndex> buildDoctorSecondaryIndex() {
    ifstream in(doctorFile);
    vector<SecondaryIndex> index;
    string line;

    while (getline(in, line)) {
        if (line.empty()) continue;

        size_t pos1 = line.find('|');
        size_t pos2 = line.find('|', pos1 + 1);
        string id = line.substr(0, pos1);
        string name = line.substr(pos1 + 1, pos2 - pos1 - 1);

        SecondaryIndex s;
        strcpy(s.key, name.c_str());
        strcpy(s.ID, id.c_str());
        index.push_back(s);
    }

    in.close();
    sort(index.begin(), index.end());
    writeSecondaryIndex(index, doctorSecondaryFile);
    return index;
}

// Create secondary index for appointments (by doctor ID)
vector<SecondaryIndex> buildAppointmentSecondaryIndex() {
    ifstream in(appointmentFile);
    vector<SecondaryIndex> index;
    string line;

    while (getline(in, line)) {
        if (line.empty()) continue;

        size_t pos1 = line.find('|');
        size_t pos2 = line.find('|', pos1 + 1);
        string appID = line.substr(0, pos1);
        string docID = line.substr(pos1 + 1, pos2 - pos1 - 1);

        SecondaryIndex s;
        strcpy(s.key, docID.c_str());
        strcpy(s.ID, appID.c_str());
        index.push_back(s);
    }

    in.close();
    sort(index.begin(), index.end());
    writeSecondaryIndex(index, appointmentSecondaryFile);
    return index;
}

//make sure indexes are updated
void updateIndexes(){
    buildDoctorPrimaryIndex();
    buildAppointmentPrimaryIndex();
    buildDoctorSecondaryIndex();
    buildAppointmentSecondaryIndex();
}

// ===================== MAIN =====================
int main() {
    cout << "Building indexes...\n";

    // Build and save all index files
    updateIndexes ();

    cout << "All indexes have been created successfully!\n";
}