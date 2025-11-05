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
const string doctorDataFile = "Doctors.txt";
const string appointmentDataFile = "Appointments.txt";
const string doctorPrimaryIndexFile = "DoctorPrimaryIndex.txt";
const string appointmentPrimaryIndexFile = "AppointmentPrimaryIndex.txt";
const string doctorSecondaryIndexFile = "DoctorSecondaryIndex.txt";
const string appointmentSecondaryIndexFile = "AppointmentSecondaryIndex.txt";

//// ===================== STRUCT DEFINITIONS =====================
struct PrimaryIndex {
    char recordID[15];
    int recordRRN;

    bool operator<(const PrimaryIndex &other) const {
        return strcmp(recordID, other.recordID) < 0;
    }
};

struct SecondaryIndex {
    char keyValue[30];
    char linkedID[15];

    bool operator<(const SecondaryIndex &other) const {
        int cmp = strcmp(keyValue, other.keyValue);
        return cmp == 0 ? strcmp(linkedID, other.linkedID) < 0 : cmp < 0;
    }
};

//// ===================== HELPER FUNCTIONS =====================
vector<string> split(const string &line, char delim = ' ') {
    vector<string> parts;
    stringstream ss(line);
    string segment;

    while (getline(ss, segment, delim)) {
        if (!segment.empty()) {
            parts.push_back(segment);
        }
    }
    return parts;
}

void safe_strcpy(char* dest, const char* src, size_t dest_size) {
    if (src == nullptr) {
        dest[0] = '\0';
        return;
    }
    strncpy(dest, src, dest_size - 1);
    dest[dest_size - 1] = '\0';
}

//// ===================== WRITE FUNCTIONS =====================
void writePrimaryIndex(const vector<PrimaryIndex> &indexList, const string &fileName) {
    ofstream out(fileName);
    for (auto &entry : indexList) {
        out << entry.recordID << "|" << entry.recordRRN << "\n";
    }
    out.close();
    cout << "DEBUG: Written " << indexList.size() << " entries to " << fileName << endl;
}

void writeSecondaryIndex(const vector<SecondaryIndex> &indexList, const string &fileName) {
    ofstream out(fileName);
    for (auto &entry : indexList) {
        out << entry.keyValue << "|" << entry.linkedID << "\n";
    }
    out.close();
    cout << "DEBUG: Written " << indexList.size() << " entries to " << fileName << endl;
}

//// ===================== READ FUNCTIONS =====================
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

        safe_strcpy(entry.recordID, id.c_str(), sizeof(entry.recordID));
        try {
            entry.recordRRN = stoi(rrnStr);
            indexList.push_back(entry);
        } catch (const exception& e) {
            continue;
        }
    }

    in.close();
    sort(indexList.begin(), indexList.end());
    cout << "DEBUG: Loaded " << indexList.size() << " entries from " << fileName << endl;
    return indexList;
}

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

        safe_strcpy(entry.keyValue, key.c_str(), sizeof(entry.keyValue));
        safe_strcpy(entry.linkedID, id.c_str(), sizeof(entry.linkedID));
        indexList.push_back(entry);
    }

    in.close();
    sort(indexList.begin(), indexList.end());
    cout << "DEBUG: Loaded " << indexList.size() << " entries from " << fileName << endl;
    return indexList;
}

//// ===================== INDEX BUILDING FUNCTIONS =====================
vector<PrimaryIndex> buildPrimaryIndex(const string &dataFile, const string &indexFile, int idFieldIndex) {
    ifstream in(dataFile);
    vector<PrimaryIndex> primaryIndex;
    string line;
    int rrn = 0;

    if (!in.is_open()) {
        cout << "DEBUG: Could not open " << dataFile << " for indexing\n";
        return primaryIndex;
    }

    while (getline(in, line)) {
        // Skip empty lines and deleted records
        if (line.empty()) {
            rrn++;
            continue;
        }

        // Check if record is deleted
        bool isDeleted = (line[0] == '*');
        string content = isDeleted ? line.substr(1) : line;

        if (content.empty()) {
            rrn++;
            continue;
        }

        auto fields = split(content);
        if (fields.size() <= idFieldIndex) {
            rrn++;
            continue;
        }

        // Only index non-deleted records
        if (!isDeleted) {
            PrimaryIndex entry;
            safe_strcpy(entry.recordID, fields[idFieldIndex].c_str(), sizeof(entry.recordID));
            entry.recordRRN = rrn;
            primaryIndex.push_back(entry);
        }

        rrn++;
    }

    in.close();
    sort(primaryIndex.begin(), primaryIndex.end());
    writePrimaryIndex(primaryIndex, indexFile);
    cout << "DEBUG: Built primary index with " << primaryIndex.size() << " entries\n";
    return primaryIndex;
}

vector<SecondaryIndex> buildSecondaryIndex(const string &dataFile, const string &indexFile,
                                           int keyFieldIndex, int linkedFieldIndex) {
    ifstream in(dataFile);
    vector<SecondaryIndex> secondaryIndex;
    string line;
    int rrn = 0;

    if (!in.is_open()) {
        cout << "DEBUG: Could not open " << dataFile << " for indexing\n";
        return secondaryIndex;
    }

    while (getline(in, line)) {
        // Skip empty lines
        if (line.empty()) {
            rrn++;
            continue;
        }

        // Check if record is deleted
        bool isDeleted = (line[0] == '*');
        string content = isDeleted ? line.substr(1) : line;

        if (content.empty()) {
            rrn++;
            continue;
        }

        auto fields = split(content);
        if (fields.size() <= max(keyFieldIndex, linkedFieldIndex)) {
            rrn++;
            continue;
        }

        // Only index non-deleted records
        if (!isDeleted) {
            SecondaryIndex entry;
            safe_strcpy(entry.keyValue, fields[keyFieldIndex].c_str(), sizeof(entry.keyValue));
            safe_strcpy(entry.linkedID, fields[linkedFieldIndex].c_str(), sizeof(entry.linkedID));
            secondaryIndex.push_back(entry);
        }

        rrn++;
    }

    in.close();
    sort(secondaryIndex.begin(), secondaryIndex.end());
    writeSecondaryIndex(secondaryIndex, indexFile);
    cout << "DEBUG: Built secondary index with " << secondaryIndex.size() << " entries\n";
    return secondaryIndex;
}

void buildAllIndexes() {
    cout << "🔄 Building all indexes...\n";

    // Doctors: Format "Name Specialty ID" - fields: [0]=Name, [1]=Specialty, [2]=ID
    buildPrimaryIndex(doctorDataFile, doctorPrimaryIndexFile, 2);
    buildSecondaryIndex(doctorDataFile, doctorSecondaryIndexFile, 0, 2);

    // Appointments: Format "ID DoctorID Date" - fields: [0]=ID, [1]=DoctorID, [2]=Date
    buildPrimaryIndex(appointmentDataFile, appointmentPrimaryIndexFile, 0);
    buildSecondaryIndex(appointmentDataFile, appointmentSecondaryIndexFile, 1, 0);

    cout << "✅ All indexes have been built successfully!\n";
}

const char DELETE_FLAG = '*';

// -------------------- Forward Declarations --------------------
struct Doctor;
struct Appointment;

// -------------------- Doctor struct --------------------
struct Doctor {
    char ID[15];
    char Name[30];
    char Specialty[30];

    Doctor() {
        strcpy(ID, "");
        strcpy(Name, "");
        strcpy(Specialty, "");
    }

    Doctor(const char* n, const char* s, const char* id) {
        safe_strcpy(Name, n, sizeof(Name));
        safe_strcpy(Specialty, s, sizeof(Specialty));
        safe_strcpy(ID, id, sizeof(ID));
    }

    static Doctor fromLine(const string &line) {
        if (line.empty()) return Doctor();

        // Handle deleted records
        bool isDeleted = (line[0] == DELETE_FLAG);
        string content = isDeleted ? line.substr(1) : line;
        if (content.empty()) return Doctor();

        auto fields = split(content);
        if (fields.size() < 3) return Doctor();

        return Doctor(fields[0].c_str(), fields[1].c_str(), fields[2].c_str());
    }

    string toLine() const {
        return string(Name) + " " + string(Specialty) + " " + string(ID);
    }

    bool isEmpty() const {
        return strlen(Name) == 0 && strlen(Specialty) == 0 && strlen(ID) == 0;
    }

    friend ostream& operator<<(ostream &os, const Doctor &d) {
        if (d.isEmpty()) {
            os << "Deleted or empty record\n";
        } else {
            os << "Name: " << d.Name << " | Specialty: " << d.Specialty << " | ID: " << d.ID << "\n";
        }
        return os;
    }
};

// -------------------- Appointment struct --------------------
struct Appointment {
    char ID[15];
    char DoctorID[15];
    char Date[30];

    Appointment() {
        strcpy(ID, "");
        strcpy(DoctorID, "");
        strcpy(Date, "");
    }

    Appointment(const char* id, const char* docID, const char* date) {
        safe_strcpy(ID, id, sizeof(ID));
        safe_strcpy(DoctorID, docID, sizeof(DoctorID));
        safe_strcpy(Date, date, sizeof(Date));
    }

    static Appointment fromLine(const string &line) {
        if (line.empty()) return Appointment();

        // Handle deleted records
        bool isDeleted = (line[0] == DELETE_FLAG);
        string content = isDeleted ? line.substr(1) : line;
        if (content.empty()) return Appointment();

        auto fields = split(content);
        if (fields.size() < 3) return Appointment();

        return Appointment(fields[0].c_str(), fields[1].c_str(), fields[2].c_str());
    }

    string toLine() const {
        return string(ID) + " " + string(DoctorID) + " " + string(Date);
    }

    bool isEmpty() const {
        return strlen(ID) == 0 && strlen(DoctorID) == 0 && strlen(Date) == 0;
    }

    friend ostream& operator<<(ostream &os, const Appointment &a) {
        if (a.isEmpty()) {
            os << "Deleted or empty record\n";
        } else {
            os << "AppointmentID: " << a.ID << " | DoctorID: " << a.DoctorID << " | Date: " << a.Date << endl;
        }
        return os;
    }
};

//// ===================== CORRECTED SEARCH FUNCTIONS =====================
int getRRNByID(const vector<PrimaryIndex> &primaryIndex, const char *id) {
    int left = 0;
    int right = primaryIndex.size() - 1;

    while (left <= right) {
        int mid = left + (right - left) / 2;
        int cmp = strcmp(id, primaryIndex[mid].recordID);

        if (cmp == 0) {
            return primaryIndex[mid].recordRRN;
        } else if (cmp < 0) {
            right = mid - 1;
        } else {
            left = mid + 1;
        }
    }
    return -1;
}

vector<string> getAllIDsByKey(const vector<SecondaryIndex> &secondaryIndex, const char *key) {
    vector<string> ids;

    if (secondaryIndex.empty()) return ids;

    // Find first occurrence using binary search
    int left = 0;
    int right = secondaryIndex.size() - 1;
    int first = -1;

    while (left <= right) {
        int mid = left + (right - left) / 2;
        int cmp = strcmp(key, secondaryIndex[mid].keyValue);

        if (cmp == 0) {
            first = mid;
            right = mid - 1; // Continue searching left
        } else if (cmp < 0) {
            right = mid - 1;
        } else {
            left = mid + 1;
        }
    }

    if (first == -1) return ids; // Not found

    // Collect all matching IDs
    for (int i = first; i < secondaryIndex.size() && strcmp(secondaryIndex[i].keyValue, key) == 0; i++) {
        ids.push_back(string(secondaryIndex[i].linkedID));
    }

    return ids;
}

//// ===================== FILE UTILITIES =====================
vector<string> readAllLines(const string &fileName) {
    vector<string> lines;
    ifstream in(fileName);
    string line;

    if (!in.is_open()) {
        cout << "DEBUG: Could not open " << fileName << " for reading\n";
        return lines;
    }

    while (getline(in, line)) {
        lines.push_back(line);
    }
    in.close();

    cout << "DEBUG: Read " << lines.size() << " lines from " << fileName << endl;
    return lines;
}

void writeAllLines(const string &fileName, const vector<string> &lines) {
    ofstream out(fileName);
    if (!out.is_open()) {
        cout << "DEBUG: ERROR - Could not open " << fileName << " for writing\n";
        return;
    }

    for (const auto &line : lines) {
        out << line << "\n";
    }
    out.close();

    cout << "DEBUG: Written " << lines.size() << " lines to " << fileName << endl;
}

void printAvail(const vector<int> &avail) {
    if (avail.empty()) {
        cout << "Avail list is empty.\n";
        return;
    }
    cout << "Available RRNs: ";
    for (size_t i = 0; i < avail.size(); ++i) {
        cout << avail[i];
        if (i + 1 < avail.size()) cout << ", ";
    }
    cout << "\n";
}

//// ===================== APPOINTMENT OPERATIONS =====================
vector<Appointment> searchAppointmentsByDoctorID(const char *doctorID,
                                                 const vector<PrimaryIndex> &apptPrimary,
                                                 const vector<SecondaryIndex> &apptSecondary) {
    vector<Appointment> appointments;
    vector<string> apptIDs = getAllIDsByKey(apptSecondary, doctorID);

    if (apptIDs.empty()) return appointments;

    vector<string> lines = readAllLines(appointmentDataFile);

    for (auto &apptID : apptIDs) {
        int rrn = getRRNByID(apptPrimary, apptID.c_str());
        if (rrn != -1 && rrn < lines.size() && !lines[rrn].empty()) {
            Appointment appt = Appointment::fromLine(lines[rrn]);
            if (!appt.isEmpty()) {
                appointments.push_back(appt);
            }
        }
    }

    return appointments;
}

bool deleteAppointmentByID(const char *id,
                           vector<PrimaryIndex> &primary,
                           vector<SecondaryIndex> &secondary,
                           vector<int> &avail) {
    int rrn = getRRNByID(primary, id);
    if (rrn == -1) {
        cout << "DEBUG: Appointment " << id << " not found in primary index\n";
        return false;
    }

    vector<string> lines = readAllLines(appointmentDataFile);
    if (rrn >= lines.size()) {
        cout << "DEBUG: RRN " << rrn << " out of bounds for appointments file\n";
        return false;
    }

    if (lines[rrn].empty() || lines[rrn][0] == DELETE_FLAG) {
        cout << "DEBUG: Appointment " << id << " already deleted\n";
        return false;
    }

    // Mark as deleted by prepending DELETE_FLAG
    lines[rrn] = string(1, DELETE_FLAG) + lines[rrn];
    writeAllLines(appointmentDataFile, lines);
    cout << "DEBUG: Marked appointment " << id << " as deleted at RRN " << rrn << endl;

    // Remove from indices
    primary.erase(remove_if(primary.begin(), primary.end(),
                            [&](const PrimaryIndex &p) { return strcmp(p.recordID, id) == 0; }), primary.end());

    secondary.erase(remove_if(secondary.begin(), secondary.end(),
                              [&](const SecondaryIndex &s) { return strcmp(s.linkedID, id) == 0; }), secondary.end());

    sort(primary.begin(), primary.end());
    sort(secondary.begin(), secondary.end());

    writePrimaryIndex(primary, appointmentPrimaryIndexFile);
    writeSecondaryIndex(secondary, appointmentSecondaryIndexFile);

    avail.push_back(rrn);
    cout << "DEBUG: Added RRN " << rrn << " to avail list, now size: " << avail.size() << endl;
    return true;
}

bool deleteAllAppointmentsForDoctor(const char *doctorID,
                                    vector<PrimaryIndex> &apptPrimary,
                                    vector<SecondaryIndex> &apptSecondary,
                                    vector<int> &apptAvail) {
    vector<string> apptIDs = getAllIDsByKey(apptSecondary, doctorID);

    if (apptIDs.empty()) {
        cout << "No appointments found for doctor " << doctorID << endl;
        return true;
    }

    cout << "Found " << apptIDs.size() << " appointment(s) for doctor " << doctorID << ". Deleting...\n";

    bool allDeleted = true;
    int deletedCount = 0;

    for (auto &apptID : apptIDs) {
        bool success = deleteAppointmentByID(apptID.c_str(), apptPrimary, apptSecondary, apptAvail);
        if (success) {
            deletedCount++;
        } else {
            allDeleted = false;
            cout << "Failed to delete appointment " << apptID << endl;
        }
    }

    cout << "Successfully deleted " << deletedCount << " appointment(s) for doctor " << doctorID << endl;
    return allDeleted;
}

int insertAppointment(const Appointment &a,
                      vector<PrimaryIndex> &primary,
                      vector<SecondaryIndex> &secondary,
                      vector<int> &avail) {
    vector<string> lines = readAllLines(appointmentDataFile);
    int rrn = -1;

    if (!avail.empty()) {
        rrn = avail.back();
        avail.pop_back();
        cout << "DEBUG: Reusing available RRN " << rrn << endl;

        if (rrn < lines.size()) {
            lines[rrn] = a.toLine();
        } else {
            // Extend the file if needed
            while (lines.size() <= rrn) {
                lines.push_back("");
            }
            lines[rrn] = a.toLine();
        }
    } else {
        rrn = lines.size();
        lines.push_back(a.toLine());
        cout << "DEBUG: Appending new record at RRN " << rrn << endl;
    }

    // Write the updated data file
    writeAllLines(appointmentDataFile, lines);

    // Update indices
    PrimaryIndex p;
    safe_strcpy(p.recordID, a.ID, sizeof(p.recordID));
    p.recordRRN = rrn;
    primary.push_back(p);

    SecondaryIndex s;
    safe_strcpy(s.keyValue, a.DoctorID, sizeof(s.keyValue));
    safe_strcpy(s.linkedID, a.ID, sizeof(s.linkedID));
    secondary.push_back(s);

    sort(primary.begin(), primary.end());
    sort(secondary.begin(), secondary.end());

    writePrimaryIndex(primary, appointmentPrimaryIndexFile);
    writeSecondaryIndex(secondary, appointmentSecondaryIndexFile);

    cout << "DEBUG: Inserted appointment " << a.ID << " at RRN " << rrn << endl;
    return rrn;
}

//// ===================== DOCTOR OPERATIONS =====================
bool insertDoctor(const Doctor &doc,
                  vector<PrimaryIndex> &primary,
                  vector<SecondaryIndex> &secondary,
                  vector<int> &avail) {
    vector<string> lines = readAllLines(doctorDataFile);
    int rrn;

    if (!avail.empty()) {
        rrn = avail.back();
        avail.pop_back();
        cout << "DEBUG: Reusing available RRN " << rrn << " for doctor\n";
        if (rrn < lines.size()) {
            lines[rrn] = doc.toLine();
        } else {
            while (lines.size() <= rrn) {
                lines.push_back("");
            }
            lines[rrn] = doc.toLine();
        }
    } else {
        rrn = lines.size();
        lines.push_back(doc.toLine());
        cout << "DEBUG: Appending new doctor at RRN " << rrn << endl;
    }

    writeAllLines(doctorDataFile, lines);

    PrimaryIndex p;
    safe_strcpy(p.recordID, doc.ID, sizeof(p.recordID));
    p.recordRRN = rrn;
    primary.push_back(p);
    sort(primary.begin(), primary.end());
    writePrimaryIndex(primary, doctorPrimaryIndexFile);

    SecondaryIndex s;
    safe_strcpy(s.keyValue, doc.Name, sizeof(s.keyValue));
    safe_strcpy(s.linkedID, doc.ID, sizeof(s.linkedID));
    secondary.push_back(s);
    sort(secondary.begin(), secondary.end());
    writeSecondaryIndex(secondary, doctorSecondaryIndexFile);

    cout << "DEBUG: Inserted doctor " << doc.ID << " at RRN " << rrn << endl;
    return true;
}

bool deleteDoctorByID(const char *id,
                      vector<PrimaryIndex> &primary,
                      vector<SecondaryIndex> &secondary,
                      vector<int> &avail,
                      vector<PrimaryIndex> &apptPrimary,
                      vector<SecondaryIndex> &apptSecondary,
                      vector<int> &apptAvail) {
    int rrn = getRRNByID(primary, id);
    if (rrn == -1) {
        cout << "DEBUG: Doctor " << id << " not found in primary index\n";
        return false;
    }

    cout << "Deleting all appointments for doctor " << id << "..." << endl;
    deleteAllAppointmentsForDoctor(id, apptPrimary, apptSecondary, apptAvail);

    vector<string> lines = readAllLines(doctorDataFile);
    if (rrn >= lines.size()) {
        cout << "DEBUG: RRN " << rrn << " out of bounds for doctors file\n";
        return false;
    }

    if (lines[rrn].empty() || lines[rrn][0] == DELETE_FLAG) {
        cout << "DEBUG: Doctor " << id << " already deleted\n";
        return false;
    }

    // Mark as deleted
    lines[rrn] = string(1, DELETE_FLAG) + lines[rrn];
    writeAllLines(doctorDataFile, lines);
    cout << "DEBUG: Marked doctor " << id << " as deleted at RRN " << rrn << endl;

    // Remove from indices
    primary.erase(remove_if(primary.begin(), primary.end(),
                            [&](const PrimaryIndex &p) { return strcmp(p.recordID, id) == 0; }), primary.end());

    secondary.erase(remove_if(secondary.begin(), secondary.end(),
                              [&](const SecondaryIndex &s) { return strcmp(s.linkedID, id) == 0; }), secondary.end());

    sort(primary.begin(), primary.end());
    sort(secondary.begin(), secondary.end());

    writePrimaryIndex(primary, doctorPrimaryIndexFile);
    writeSecondaryIndex(secondary, doctorSecondaryIndexFile);

    avail.push_back(rrn);
    cout << "DEBUG: Added RRN " << rrn << " to doctor avail list\n";
    return true;
}

//// ===================== MENU FUNCTIONS =====================
void manageDoctors(vector<PrimaryIndex> &primary,
                   vector<SecondaryIndex> &secondary,
                   vector<int> &avail,
                   vector<PrimaryIndex> &apptPrimary,
                   vector<SecondaryIndex> &apptSecondary,
                   vector<int> &apptAvail) {
    char choice;
    do {
        cout << "\n--- Manage Doctors ---\n";
        cout << "I: Insert  V: View by RRN  D: Delete by ID  S: Search by ID  N: Search by Name  A: Print Avail  P: Search Appointments by Doctor ID  B: Back\n";
        cout << "Choose: ";
        cin >> choice;

        if (choice == 'I' || choice == 'i') {
            Doctor d;
            cout << "Enter Name (no spaces): "; cin >> d.Name;
            cout << "Enter Specialty (no spaces): "; cin >> d.Specialty;
            cout << "Enter ID: "; cin >> d.ID;
            bool success = insertDoctor(d, primary, secondary, avail);
            cout << (success ? "Inserted successfully\n" : "Insertion failed\n");
        } else if (choice == 'V' || choice == 'v') {
            int r; cout << "Enter RRN: "; cin >> r;
            vector<string> lines = readAllLines(doctorDataFile);
            if (r < 0 || r >= lines.size()) {
                cout << "Invalid RRN\n";
            } else if (lines[r].empty()) {
                cout << "Empty record\n";
            } else {
                Doctor d = Doctor::fromLine(lines[r]);
                cout << d;
            }
        } else if (choice == 'D' || choice == 'd') {
            char id[15]; cout << "Enter ID to delete: "; cin >> id;
            bool ok = deleteDoctorByID(id, primary, secondary, avail, apptPrimary, apptSecondary, apptAvail);
            cout << (ok ? "Deleted\n" : "Not found or already deleted\n");
        } else if (choice == 'S' || choice == 's') {
            char id[15]; cout << "Enter ID: "; cin >> id;
            int rrn = getRRNByID(primary, id);
            if (rrn == -1) {
                cout << "ID not found\n";
            } else {
                vector<string> lines = readAllLines(doctorDataFile);
                if (rrn < lines.size() && !lines[rrn].empty()) {
                    Doctor d = Doctor::fromLine(lines[rrn]);
                    cout << d;
                } else {
                    cout << "Deleted or not present in file\n";
                }
            }
        } else if (choice == 'N' || choice == 'n') {
            char name[30]; cout << "Enter Name (exact): ";
            cin.ignore();
            cin.getline(name, 30);
            vector<string> ids = getAllIDsByKey(secondary, name);
            if (ids.empty()) {
                cout << "No records with that name\n";
            } else {
                vector<string> lines = readAllLines(doctorDataFile);
                for (auto &id : ids) {
                    int rrn = getRRNByID(primary, id.c_str());
                    if (rrn != -1 && rrn < lines.size() && !lines[rrn].empty()) {
                        Doctor d = Doctor::fromLine(lines[rrn]);
                        if (!d.isEmpty()) {
                            cout << d;
                        }
                    }
                }
            }
        } else if (choice == 'A' || choice == 'a') {
            printAvail(avail);
        } else if (choice == 'P' || choice == 'p') {
            char doctorID[15]; cout << "Enter Doctor ID to search appointments: "; cin >> doctorID;
            vector<Appointment> appointments = searchAppointmentsByDoctorID(doctorID, apptPrimary, apptSecondary);
            if (appointments.empty()) {
                cout << "No appointments found for doctor " << doctorID << endl;
            } else {
                cout << "Found " << appointments.size() << " appointment(s) for doctor " << doctorID << ":\n";
                for (const auto &appt : appointments) {
                    cout << appt;
                }
            }
        }
    } while (choice != 'B' && choice != 'b');
}

void manageAppointments(vector<PrimaryIndex> &primary,
                        vector<SecondaryIndex> &secondary,
                        vector<int> &avail) {
    char choice;
    do {
        cout << "\n--- Manage Appointments ---\n";
        cout << "I: Insert  V: View by RRN  D: Delete by ID  S: Search by ID  N: Search by DoctorID  A: Print Avail  B: Back\n";
        cout << "Choose: ";
        cin >> choice;

        if (choice == 'I' || choice == 'i') {
            Appointment a;
            cout << "Enter AppointmentID: "; cin >> a.ID;
            cout << "Enter DoctorID: "; cin >> a.DoctorID;
            cout << "Enter Date (no spaces): "; cin >> a.Date;
            int rrn = insertAppointment(a, primary, secondary, avail);
            cout << "Inserted at RRN: " << rrn << "\n";
        } else if (choice == 'V' || choice == 'v') {
            int r; cout << "Enter RRN: "; cin >> r;
            vector<string> lines = readAllLines(appointmentDataFile);
            if (r < 0 || r >= lines.size()) {
                cout << "Invalid RRN\n";
            } else if (lines[r].empty()) {
                cout << "Empty record\n";
            } else {
                Appointment a = Appointment::fromLine(lines[r]);
                cout << a;
            }
        } else if (choice == 'D' || choice == 'd') {
            char id[15]; cout << "Enter Appointment ID to delete: "; cin >> id;
            bool ok = deleteAppointmentByID(id, primary, secondary, avail);
            cout << (ok ? "Deleted\n" : "Not found or already deleted\n");
        } else if (choice == 'S' || choice == 's') {
            char id[15]; cout << "Enter Appointment ID: "; cin >> id;
            int rrn = getRRNByID(primary, id);
            if (rrn == -1) {
                cout << "ID not found\n";
            } else {
                vector<string> lines = readAllLines(appointmentDataFile);
                if (rrn < lines.size() && !lines[rrn].empty()) {
                    Appointment a = Appointment::fromLine(lines[rrn]);
                    cout << a;
                } else {
                    cout << "Deleted or not present in file\n";
                }
            }
        } else if (choice == 'N' || choice == 'n') {
            char docid[15]; cout << "Enter DoctorID: ";
            cin.ignore();
            cin.getline(docid, 15);
            vector<string> ids = getAllIDsByKey(secondary, docid);
            if (ids.empty()) {
                cout << "No appointments for that doctor\n";
            } else {
                vector<string> lines = readAllLines(appointmentDataFile);
                for (auto &id : ids) {
                    int rrn = getRRNByID(primary, id.c_str());
                    if (rrn != -1 && rrn < lines.size() && !lines[rrn].empty()) {
                        Appointment a = Appointment::fromLine(lines[rrn]);
                        if (!a.isEmpty()) {
                            cout << a;
                        }
                    }
                }
            }
        } else if (choice == 'A' || choice == 'a') {
            printAvail(avail);
        }
    } while (choice != 'B' && choice != 'b');
}

//// ===================== MAIN FUNCTION =====================
int main() {
    // Ensure data files exist
    ofstream doctorFile(doctorDataFile, ios::app);
    ofstream appointmentFile(appointmentDataFile, ios::app);
    doctorFile.close();
    appointmentFile.close();

    cout << "=== Loading Index Files ===\n";

    // Load indices
    vector<PrimaryIndex> doctorPrimary = readPrimaryIndex(doctorPrimaryIndexFile);
    vector<SecondaryIndex> doctorSecondary = readSecondaryIndex(doctorSecondaryIndexFile);
    vector<PrimaryIndex> apptPrimary = readPrimaryIndex(appointmentPrimaryIndexFile);
    vector<SecondaryIndex> apptSecondary = readSecondaryIndex(appointmentSecondaryIndexFile);

    vector<int> doctorAvail;
    vector<int> apptAvail;

    char mainChoice;
    do {
        cout << "\n=== Main Menu ===\n";
        cout << "1. Manage Doctors\n";
        cout << "2. Manage Appointments\n";
        cout << "3. Build/Rebuild All Indexes\n";
        cout << "E. Exit\n";
        cout << "Choose: ";
        cin >> mainChoice;

        if (mainChoice == '1') {
            manageDoctors(doctorPrimary, doctorSecondary, doctorAvail, apptPrimary, apptSecondary, apptAvail);
        } else if (mainChoice == '2') {
            manageAppointments(apptPrimary, apptSecondary, apptAvail);
        } else if (mainChoice == '3') {
            buildAllIndexes();
            // Reload indexes
            doctorPrimary = readPrimaryIndex(doctorPrimaryIndexFile);
            doctorSecondary = readSecondaryIndex(doctorSecondaryIndexFile);
            apptPrimary = readPrimaryIndex(appointmentPrimaryIndexFile);
            apptSecondary = readSecondaryIndex(appointmentSecondaryIndexFile);
        }
    } while (mainChoice != 'E' && mainChoice != 'e');

    cout << "Goodbye\n";
    return 0;
}
