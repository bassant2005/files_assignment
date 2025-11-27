#include "creating_data_files.cpp"
#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <algorithm>
#include <string>

using namespace std;

const char DELETE_FLAG = '*';

// Safe copy helper: prevents buffer overflow
void safe_strcpy(char* dest, const char* src, size_t size) {
    strncpy(dest, src, size - 1);
    dest[size - 1] = '\0';
}

/**
 * ===============================================================
 *  GENERAL Struct for Doctor and Appointment & GENERAL DELETE & SEARCH functions
 *  created by: Habeba Hosam
 *  Purpose: Implements the logic for deleting & searching
 *           functions for both Doctor & Appointment.
 *  Student ID: 20230117
 * ===============================================================
 */

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

        bool isDeleted = (line[0] == DELETE_FLAG);
        string content = isDeleted ? line.substr(1) : line;

        auto fields = split(content);

        if (fields.size() < 4) return Doctor();

        return Doctor(fields[1].c_str(), fields[2].c_str(), fields[3].c_str());
    }


    string toLine() const {
        string data = string(Name) + "|" + string(Specialty) + "|" + string(ID);
        return to_string(data.length()) + "|" + data;
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

        auto fields = split(content);
        if (fields.size() < 4) return Appointment();

        return Appointment(fields[1].c_str(), fields[2].c_str(), fields[3].c_str());
    }

    string toLine() const {
        string data = string(ID) + "|" + string(DoctorID) + "|" + string(Date);
        return to_string(data.length()) + "|" + data;
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

static string readLineAtOffset(const string &fileName, long long offset) {
    ifstream in(fileName, ios::binary);
    if (!in.is_open()) return "";
    in.seekg(offset);
    string line;
    getline(in, line);
    return line;
}

// Offset-based helpers
long long getOffsetByID(const vector<PrimaryIndex> &primaryIndex, const char *id) {
    int left = 0;
    int right = (int)primaryIndex.size() - 1;

    while (left <= right) {
        int mid = left + (right - left) / 2;
        int cmp = strcmp(id, primaryIndex[mid].recordID);
        if (cmp == 0) return primaryIndex[mid].offset;
        if (cmp < 0) right = mid - 1; else left = mid + 1;
    }
    return -1;
}

//// ===================== SEARCH FUNCTIONS =====================
int getRRNByID(const vector<PrimaryIndex> &primaryIndex, const char *id) {
    int left = 0;
    int right = primaryIndex.size() - 1;

    while (left <= right) {
        int mid = left + (right - left) / 2;
        int cmp = strcmp(id, primaryIndex[mid].recordID);

        if (cmp == 0) {
            return primaryIndex[mid].recordLength;
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
        const char* currentKey = secondaryIndex[mid].keyValue;
        // Skip deleted records in comparison
        const char* compareKey = (currentKey[0] == '*') ? currentKey + 1 : currentKey;
        
        int cmp = strcmp(key, compareKey);

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

    // Collect all matching IDs (including deleted ones, they'll be filtered later)
    for (int i = first; i < secondaryIndex.size(); i++) {
        const char* currentKey = secondaryIndex[i].keyValue;
        const char* compareKey = (currentKey[0] == '*') ? currentKey + 1 : currentKey;
        
        if (strcmp(key, compareKey) != 0) break;
        
        // Only add non-deleted records
        if (secondaryIndex[i].linkedID[0] != '*') {
            ids.push_back(secondaryIndex[i].linkedID);
        }
    }

    return ids;
}

long long resolveActiveOffsetForID(const vector<PrimaryIndex> &primaryIndex,
                                   const string &dataFile,
                                   const char *id) {
    int left = 0;
    int right = (int)primaryIndex.size() - 1;
    int first = -1;
    while (left <= right) {
        int mid = left + (right - left) / 2;
        const char* idx = primaryIndex[mid].recordID;
        const char* cmpID = (idx[0] == '*') ? idx + 1 : idx;
        int cmp = strcmp(id, cmpID);
        if (cmp == 0) { first = mid; right = mid - 1; }
        else if (cmp < 0) right = mid - 1; else left = mid + 1;
    }
    if (first == -1) return -1;
    for (int i = first; i < (int)primaryIndex.size(); ++i) {
        const char* idx = primaryIndex[i].recordID;
        const char* cmpID = (idx[0] == '*') ? idx + 1 : idx;
        if (strcmp(id, cmpID) != 0) break;
        long long off = primaryIndex[i].offset;
        string line = readLineAtOffset(dataFile, off);
        if (!line.empty() && line[0] != DELETE_FLAG) {
            return off;
        }
    }
    return -1;
}

//// ===================== FILE UTILITIES =====================
vector<string> readAllLines(const string &fileName) {
    vector<string> lines;
    ifstream in(fileName);
    string line;

    if (!in.is_open()) {
        cout << "Could not open " << fileName << " for reading\n";
        return lines;
    }

    while (getline(in, line)) {
        lines.push_back(line);
    }
    in.close();

    return lines;
}

void writeAllLines(const string &fileName, const vector<string> &lines) {
    ofstream out(fileName, ios::binary);  // Open in binary mode to prevent newline conversion
    if (!out.is_open()) {
        cout << "ERROR - Could not open " << fileName << " for writing\n";
        return;
    }

    for (size_t i = 0; i < lines.size(); ++i) {
        out << lines[i];
        // Only add newline if it's not the last line
        if (i != lines.size() - 1) {
            out << "\n";
        }
    }
    out.close();
}

// Map an offset back to line index (RRN) by cumulative lengths
static int rrnFromOffset(const vector<string> &lines, long long off){
    long long cur = 0;
    for(int i=0;i<(int)lines.size();++i){
        if(cur == off) return i;
        cur += (long long)lines[i].length() + 1;
    }
    return -1;
}

static vector<long long> computeLineOffsets(const string &fileName){
    vector<long long> offsets;
    ifstream in(fileName, ios::binary);
    if(!in.is_open()) return offsets;
    string line;
    while(true){
        long long start = (long long)in.tellg();
        if(!getline(in, line)) break;
        offsets.push_back(start);
    }
    in.close();
    return offsets;
}

static int rrnFromOffsetBinary(const string &fileName, long long off){
    vector<long long> offs = computeLineOffsets(fileName);
    for(int i=0;i<(int)offs.size();++i){
        if(offs[i] == off) return i;
    }
    return -1;
}

static int rrnFromApproxOffsetBinary(const string &fileName, long long off){
    vector<long long> offs = computeLineOffsets(fileName);
    if(offs.empty()) return -1;
    for(int i=0;i<(int)offs.size()-1;++i){
        if(off >= offs[i] && off < offs[i+1]) return i;
    }
    if(off >= offs.back()) return (int)offs.size()-1;
    return -1;
}

static bool prependDeleteFlagAtOffset(const string &fileName, long long offset) {
    ifstream in(fileName, ios::binary);
    if (!in.is_open()) return false;
    string content((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
    in.close();

    if (offset < 0 || offset > (long long)content.size()) return false;
    if (offset < (long long)content.size() && content[offset] == DELETE_FLAG) return false;

    content.insert((size_t)offset, 1, DELETE_FLAG);

    ofstream out(fileName, ios::binary | ios::trunc);
    if (!out.is_open()) return false;
    out.write(content.data(), (streamsize)content.size());
    out.close();
    return true;
}

//// ===================== APPOINTMENT OPERATIONS =====================
vector<Appointment> searchAppointmentsByDoctorID(const char *doctorID,
                                                 const vector<PrimaryIndex> &apptPrimary,
                                                 const vector<SecondaryIndex> &apptSecondary) {
    vector<Appointment> appointments;
    vector<string> apptIDs = getAllIDsByKey(apptSecondary, doctorID);

    if (apptIDs.empty()) return appointments;



    for (auto &apptID : apptIDs) {
        long long off = getOffsetByID(apptPrimary, apptID.c_str());
        if (off != -1) {
            string line = readLineAtOffset(appointmentDataFile, off);
            if (!line.empty() && line[0] != DELETE_FLAG) {
                Appointment appt = Appointment::fromLine(line);
                if (!appt.isEmpty()) appointments.push_back(appt);
            }
        }
    }

    return appointments;
}

bool deleteAppointmentByID(const char *id,
                           vector<PrimaryIndex> &primary,
                           vector<SecondaryIndex> &secondary,
                           vector<int> &avail) {
    // Find the offset of the record to delete
    long long off = -1;
    bool isDeletedInIndex = false;
    
    // Find the record in primary index
    for (const auto &entry : primary) {
        // Check for deleted entries in the index
        if (entry.recordID[0] == '*') {
            if (strcmp(entry.recordID + 1, id) == 0) {
                isDeletedInIndex = true;
                off = entry.offset;
                break;
            }
        } else if (strcmp(entry.recordID, id) == 0) {
            off = entry.offset;
            break;
        }
    }
    
    if (off == -1) {
        cout << "Appointment " << id << " not found\n";
        return false;
    }

    // Read the record to verify its current state
    string line = readLineAtOffset(appointmentDataFile, off);
    if (line.empty()) {
        cout << "Error: Could not read appointment data\n";
        return false;
    }
    
    // Check if the record is already deleted in the data file
    if (line[0] == DELETE_FLAG) {
        // If it's deleted in the data file but not in the index, update the index
        if (!isDeletedInIndex) {
            for (auto &entry : primary) {
                if (strcmp(entry.recordID, id) == 0) {
                    string deletedID = string("*") + id;
                    strcpy(entry.recordID, deletedID.c_str());
                    writePrimaryIndex(primary, appointmentPrimaryIndexFile);
                    break;
                }
            }
        }
        cout << "Appointment " << id << " is already deleted\n";
        return false;
    }

    if (!prependDeleteFlagAtOffset(appointmentDataFile, off)) {
        cout << "Failed to mark appointment as deleted\n";
        return false;
    }
    
    // Update the primary index to mark this record as deleted
    for (auto &entry : primary) {
        if (strcmp(entry.recordID, id) == 0) {
            // Mark as deleted in primary index
            string deletedID = string("*") + id;
            strcpy(entry.recordID, deletedID.c_str());
            break;
        }
    }
    
    // Update the secondary index to mark related entries as deleted
    for (auto &entry : secondary) {
        if (strcmp(entry.linkedID, id) == 0) {
            // Mark the linked ID as deleted
            string deletedID = string("*") + id;
            strcpy(entry.linkedID, deletedID.c_str());
        }
    }
    
    writePrimaryIndex(primary, appointmentPrimaryIndexFile);
    writeSecondaryIndex(secondary, appointmentSecondaryIndexFile);

    Build_indexes();
    primary = readPrimaryIndex(appointmentPrimaryIndexFile);
    secondary = readSecondaryIndex(appointmentSecondaryIndexFile);
    cout << "Successfully deleted appointment " << id << "\n";
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

//// ===================== DOCTOR OPERATIONS =====================
bool deleteDoctorByID(const char *id,
                      vector<PrimaryIndex> &primary,
                      vector<SecondaryIndex> &secondary,
                      vector<int> &avail,
                      vector<PrimaryIndex> &apptPrimary,
                      vector<SecondaryIndex> &apptSecondary,
                      vector<int> &apptAvail) {
    // Find the doctor's offset
    long long off = -1;
    bool isDeletedInIndex = false;
    
    // First check if the ID exists in the primary index and get its offset
    for (const auto &entry : primary) {
        // Skip already deleted entries in the index
        if (entry.recordID[0] == '*') {
            if (strcmp(entry.recordID + 1, id) == 0) {
                isDeletedInIndex = true;
                off = entry.offset;
                break;
            }
        } else if (strcmp(entry.recordID, id) == 0) {
            off = entry.offset;
            break;
        }
    }
    
    if (off == -1) {
        cout << "Doctor " << id << " not found\n";
        return false;
    }

    // Read the record to verify its current state
    string line = readLineAtOffset(doctorDataFile, off);
    if (line.empty()) {
        cout << "Error: Could not read doctor data\n";
        return false;
    }
    
    // Check if the record is already deleted in the data file
    if (line[0] == DELETE_FLAG) {
        // If it's deleted in the data file but not in the index, update the index
        if (!isDeletedInIndex) {
            for (auto &entry : primary) {
                if (strcmp(entry.recordID, id) == 0) {
                    string deletedID = string("*") + id;
                    strcpy(entry.recordID, deletedID.c_str());
                    writePrimaryIndex(primary, doctorPrimaryIndexFile);
                    break;
                }
            }
        }
        cout << "Doctor " << id << " is already deleted\n";
        return false;
    }

    // First delete all appointments for this doctor
    cout << "Deleting all appointments for doctor " << id << "...\n";
    deleteAllAppointmentsForDoctor(id, apptPrimary, apptSecondary, apptAvail);

    if (!prependDeleteFlagAtOffset(doctorDataFile, off)) {
        cout << "Failed to mark doctor as deleted in data file\n";
        return false;
    }

    // Update primary index to mark as deleted
    for (auto &entry : primary) {
        if (strcmp(entry.recordID, id) == 0) {
            string deletedID = string("*") + id;
            strcpy(entry.recordID, deletedID.c_str());
            break;
        }
    }

    // Update secondary index to mark as deleted
    for (auto &entry : secondary) {
        if (strcmp(entry.linkedID, id) == 0) {
            string deletedID = string("*") + id;
            strcpy(entry.linkedID, deletedID.c_str());
        }
    }

    // Write updated indexes
    writePrimaryIndex(primary, doctorPrimaryIndexFile);
    writeSecondaryIndex(secondary, doctorSecondaryIndexFile);

    Build_indexes();
    primary = readPrimaryIndex(doctorPrimaryIndexFile);
    secondary = readSecondaryIndex(doctorSecondaryIndexFile);
    apptPrimary = readPrimaryIndex(appointmentPrimaryIndexFile);
    apptSecondary = readSecondaryIndex(appointmentSecondaryIndexFile);
    cout << "Successfully deleted doctor " << id << " and all related appointments\n";
    return true;
}

/**
 * ===============================================================
 *  GENERAL ADD & UPDATE created by: Nour Hany
 *  Purpose: Implements the logic for building the add & update
 *           functions for both Doctor & Appointment.
 *  Student ID: 20230447
 * ===============================================================
 */

// ---------- Add New Doctor ----------
bool addDoctor(vector<PrimaryIndex> &primary,vector<SecondaryIndex> &secondary,vector<int> &avail) {
    Doctor d;
    cout << "Enter Doctor Name: "; cin >> d.Name;
    cout << "Enter Specialty: "; cin >> d.Specialty;
    cout << "Enter Doctor ID: "; cin >> d.ID;

    // Refresh indexes and verify existence only if active (not deleted)
    primary = readPrimaryIndex(doctorPrimaryIndexFile);
    long long eoff = resolveActiveOffsetForID(primary, doctorDataFile, d.ID);
    if (eoff != -1) {
        cout << "Doctor with this ID already exists.\n";
        return false;
    }

    vector<string> lines = readAllLines(doctorDataFile);
    int rrn;

    // Reuse from avail list if possible
    if (!avail.empty()) {
        rrn = avail.back();
        avail.pop_back();
        if (rrn < lines.size()) lines[rrn] = d.toLine();
        else {
            while (lines.size() <= rrn) lines.push_back("");
            lines[rrn] = d.toLine();
        }
    } else {
        rrn = lines.size();
        lines.push_back(d.toLine());
    }

    // Write doctor record
    writeAllLines(doctorDataFile, lines);

    // Update primary index using offset
    long long offset = 0;
    for (int i = 0; i < rrn; ++i) offset += (long long)lines[i].length() + 1;
    PrimaryIndex p;
    safe_strcpy(p.recordID, d.ID, sizeof(p.recordID));
    p.offset = (int)offset;
    primary.push_back(p);
    sort(primary.begin(), primary.end());
    writePrimaryIndex(primary, doctorPrimaryIndexFile);

    // Update secondary index
    SecondaryIndex s;
    safe_strcpy(s.keyValue, d.Name, sizeof(s.keyValue));
    safe_strcpy(s.linkedID, d.ID, sizeof(s.linkedID));
    secondary.push_back(s);
    sort(secondary.begin(), secondary.end());
    writeSecondaryIndex(secondary, doctorSecondaryIndexFile);

    Build_indexes();
    primary = readPrimaryIndex(doctorPrimaryIndexFile);
    secondary = readSecondaryIndex(doctorSecondaryIndexFile);
    cout << "Doctor added successfully" << endl;
    return true;
}

// ---------- Add New Appointment ----------
bool addAppointment(vector<PrimaryIndex> &primary,vector<SecondaryIndex> &secondary,vector<int> &avail, const vector<PrimaryIndex> &doctorPrimary) {
    Appointment a;
    cout << "Enter Appointment ID: "; cin >> a.ID;
    cout << "Enter Doctor ID: "; cin >> a.DoctorID;
    cout << "Enter Date (no spaces): "; cin >> a.Date;

    // Refresh and verify existence only if active (not deleted)
    primary = readPrimaryIndex(appointmentPrimaryIndexFile);
    long long aoff = resolveActiveOffsetForID(primary, appointmentDataFile, a.ID);
    if (aoff != -1) {
        cout << "Appointment already exists.\n";
        return false;
    }

    // Validate DoctorID exists and not deleted
    long long doff = resolveActiveOffsetForID(doctorPrimary, doctorDataFile, a.DoctorID);
    if (doff == -1) {
        cout << "Doctor does not exist. Cannot add appointment.\n";
        return false;
    }
    {
        string dline = readLineAtOffset(doctorDataFile, doff);
        if (dline.empty() || dline[0] == DELETE_FLAG) {
            cout << "Doctor record is deleted or invalid. Cannot add appointment.\n";
            return false;
        }
    }

    vector<string> lines = readAllLines(appointmentDataFile);
    int rrn;
    if (!avail.empty()) {
        rrn = avail.back();
        avail.pop_back();
        if (rrn < (int)lines.size()) {
            lines[rrn] = a.toLine();
        } else {
            while ((int)lines.size() <= rrn) lines.push_back("");
            lines[rrn] = a.toLine();
        }
    } else {
        rrn = (int)lines.size();
        lines.push_back(a.toLine());
    }

    writeAllLines(appointmentDataFile, lines);

    long long offset = 0;
    for (int i = 0; i < rrn; ++i) offset += (long long)lines[i].length() + 1;
    PrimaryIndex p;
    safe_strcpy(p.recordID, a.ID, sizeof(p.recordID));
    p.offset = (int)offset;
    primary.push_back(p);
    sort(primary.begin(), primary.end());
    writePrimaryIndex(primary, appointmentPrimaryIndexFile);

    // Update secondary index for doctor ID
    SecondaryIndex s;
    safe_strcpy(s.keyValue, a.DoctorID, sizeof(s.keyValue));
    safe_strcpy(s.linkedID, a.ID, sizeof(s.linkedID));

    // Read current secondary index to avoid duplicates
    secondary = readSecondaryIndex(appointmentSecondaryIndexFile);

    // Check if this doctor-appointment mapping already exists
    bool exists = false;
    for (const auto& entry : secondary) {
        if (strcmp(entry.keyValue, a.DoctorID) == 0 &&
            strcmp(entry.linkedID, a.ID) == 0) {
            exists = true;
            break;
        }
    }

    if (!exists) {
        secondary.push_back(s);
        sort(secondary.begin(), secondary.end());
        writeSecondaryIndex(secondary, appointmentSecondaryIndexFile);
    }
    cout << "Appointment added successfully" << endl;
    return true;
}

// ---------- Update Existing Doctor ----------
bool updateDoctor(vector<PrimaryIndex> &primary,vector<SecondaryIndex> &secondary) {
    char id[15];
    cout << "Enter Doctor ID to update: ";
    cin >> id;

    long long off = resolveActiveOffsetForID(primary, doctorDataFile, id);
    if (off == -1) {
        cout << "Doctor not found.\n";
        return false;
    }

    vector<string> lines = readAllLines(doctorDataFile);
    int rrn = rrnFromOffset(lines, off);
    if (rrn == -1 || lines[rrn].empty() || lines[rrn][0] == DELETE_FLAG) {
        cout << "Invalid or deleted record.\n";
        return false;
    }

    Doctor d = Doctor::fromLine(lines[rrn]);
    cout << "Current: " << d;

    cout << "Enter new Name: ";
    cin >> d.Name;
    cout << "Enter new Specialty: ";
    cin >> d.Specialty;

    lines[rrn] = d.toLine();


    ofstream out(doctorDataFile);
    for (auto &line : lines){
        out << line << "\n";
    }
    out.close();

    // Update secondary index -> Dr.Name
    for (auto &s : secondary) {
        if (strcmp(s.linkedID, id) == 0) {
            safe_strcpy(s.keyValue, d.Name, sizeof(s.keyValue));
        }
    }

    sort(secondary.begin(), secondary.end());
    writeSecondaryIndex(secondary, doctorSecondaryIndexFile);

    Build_indexes();
    primary = readPrimaryIndex(doctorPrimaryIndexFile);
    secondary = readSecondaryIndex(doctorSecondaryIndexFile);
    cout << "Doctor updated successfully.\n";
    return true;
}

// ---------- Update Existing Appointment ----------
bool updateAppointment(vector<PrimaryIndex> &primary,vector<SecondaryIndex> &secondary, const vector<PrimaryIndex> &doctorPrimary) {
    char id[15];
    cout << "Enter Appointment ID to update: "; cin >> id;
    long long off = resolveActiveOffsetForID(primary, appointmentDataFile, id);
    if (off == -1) {
        cout << "Appointment not found.\n";
        return false;
    }

    vector<string> lines = readAllLines(appointmentDataFile);
    int rrn = rrnFromOffset(lines, off);
    if (rrn == -1) { cout << "Invalid record position.\n"; return false; }
    Appointment a = Appointment::fromLine(lines[rrn]);
    cout << "Current: " << a;
    cout << "Enter new DoctorID: "; cin >> a.DoctorID;
    cout << "Enter new Date: "; cin >> a.Date;

    // Validate new DoctorID (must be active)
    long long doff = resolveActiveOffsetForID(doctorPrimary, doctorDataFile, a.DoctorID);
    if (doff == -1) {
        cout << "Doctor does not exist. Update aborted.\n";
        return false;
    }
    {
        string dline = readLineAtOffset(doctorDataFile, doff);
        if (dline.empty() || dline[0] == DELETE_FLAG) {
            cout << "Doctor record is deleted or invalid. Update aborted.\n";
            return false;
        }
    }

    lines[rrn] = a.toLine();
    writeAllLines(appointmentDataFile, lines);

    // Update secondary index (DoctorID)
    for (auto &s : secondary)
        if (strcmp(s.linkedID, id) == 0)
            safe_strcpy(s.keyValue, a.DoctorID, sizeof(s.keyValue));

    sort(secondary.begin(), secondary.end());
    writeSecondaryIndex(secondary, appointmentSecondaryIndexFile);

    Build_indexes();
    primary = readPrimaryIndex(appointmentPrimaryIndexFile);
    secondary = readSecondaryIndex(appointmentSecondaryIndexFile);
    cout << "Appointment updated successfully.\n";
    return true;
}
