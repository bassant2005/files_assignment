#include "creating_data_files.cpp"
#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <algorithm>
#include <string>

using namespace std;

const char DELETE_FLAG = '*';
/**
 * ===============================================================
 *  GENERAL Struct for Doctor and Appointment & GENERAL DELETE & SEARCH functions
 *  created by: Habeba Hosam
 *  Purpose: Implements the logic for deleting & searching
 *           functions for both Doctor & Appointment.
 *  Student ID: 20230117
 * ===============================================================
 */

// Safe copy helper: prevents buffer overflow
void safe_strcpy(char* dest, const char* src, size_t size) {
    strncpy(dest, src, size - 1);
    dest[size - 1] = '\0';
}


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

//// ===================== CORRECTED SEARCH FUNCTIONS =====================
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

//// ===================== DOCTOR OPERATIONS =====================

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

    // Check if ID already exists
    if (getRRNByID(primary, d.ID) != -1) {
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

    // Update primary index
    PrimaryIndex p;
    safe_strcpy(p.recordID, d.ID, sizeof(p.recordID));
    p.recordLength = rrn;
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

    cout << "Doctor added successfully at RRN " << rrn << endl;
    return true;
}

// ---------- Add New Appointment ----------
bool addAppointment(vector<PrimaryIndex> &primary,vector<SecondaryIndex> &secondary,vector<int> &avail, const vector<PrimaryIndex> &doctorPrimary) {
    Appointment a;
    cout << "Enter Appointment ID: "; cin >> a.ID;
    cout << "Enter Doctor ID: "; cin >> a.DoctorID;
    cout << "Enter Date (no spaces): "; cin >> a.Date;

    // Check if Appointment ID exists
    if (getRRNByID(primary, a.ID) != -1) {
        cout << "Appointment already exists.\n";
        return false;
    }

    // Validate DoctorID exists and not deleted
    int drrn = getRRNByID(doctorPrimary, a.DoctorID);
    if (drrn == -1) {
        cout << "Doctor does not exist. Cannot add appointment.\n";
        return false;
    }
    {
        vector<string> dlines = readAllLines(doctorDataFile);
        if (drrn < 0 || drrn >= (int)dlines.size() || dlines[drrn].empty() || dlines[drrn][0] == DELETE_FLAG) {
            cout << "Doctor record is deleted or invalid. Cannot add appointment.\n";
            return false;
        }
    }

    vector<string> lines = readAllLines(appointmentDataFile);
    int rrn;

    if (!avail.empty()) {
        rrn = avail.back();
        avail.pop_back();
        if (rrn < lines.size()) lines[rrn] = a.toLine();
        else {
            while (lines.size() <= rrn) lines.push_back("");
            lines[rrn] = a.toLine();
        }
    } else {
        rrn = lines.size();
        lines.push_back(a.toLine());
    }

    writeAllLines(appointmentDataFile, lines);

    // Update indices
    PrimaryIndex p;
    safe_strcpy(p.recordID, a.ID, sizeof(p.recordID));
    p.recordLength = rrn;
    primary.push_back(p);
    sort(primary.begin(), primary.end());
    writePrimaryIndex(primary, appointmentPrimaryIndexFile);

    SecondaryIndex s;
    safe_strcpy(s.keyValue, a.DoctorID, sizeof(s.keyValue));
    safe_strcpy(s.linkedID, a.ID, sizeof(s.linkedID));
    secondary.push_back(s);
    sort(secondary.begin(), secondary.end());
    writeSecondaryIndex(secondary, appointmentSecondaryIndexFile);
    secondary = readSecondaryIndex(appointmentSecondaryIndexFile);

    cout << "Appointment added successfully at RRN " << rrn << endl;
    return true;
}

// ---------- Update Existing Doctor ----------
bool updateDoctor(vector<PrimaryIndex> &primary,vector<SecondaryIndex> &secondary) {
    char id[15];
    cout << "Enter Doctor ID to update: ";
    cin >> id;

    int rrn = getRRNByID(primary, id);
    if (rrn == -1) {
        cout << "Doctor not found.\n";
        return false;
    }

    vector<string> lines = readAllLines(doctorDataFile);
    if (rrn < 0 || rrn >= lines.size() || lines[rrn].empty() || lines[rrn][0] == DELETE_FLAG) {
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

    cout << "Doctor updated successfully.\n";
    return true;
}



// ---------- Update Existing Appointment ----------
bool updateAppointment(vector<PrimaryIndex> &primary,vector<SecondaryIndex> &secondary, const vector<PrimaryIndex> &doctorPrimary) {
    char id[15];
    cout << "Enter Appointment ID to update: "; cin >> id;
    int rrn = getRRNByID(primary, id);
    if (rrn == -1) {
        cout << "Appointment not found.\n";
        return false;
    }

    vector<string> lines = readAllLines(appointmentDataFile);
    Appointment a = Appointment::fromLine(lines[rrn]);
    cout << "Current: " << a;
    cout << "Enter new DoctorID: "; cin >> a.DoctorID;
    cout << "Enter new Date: "; cin >> a.Date;

    // Validate new DoctorID
    int drrn = getRRNByID(doctorPrimary, a.DoctorID);
    if (drrn == -1) {
        cout << "Doctor does not exist. Update aborted.\n";
        return false;
    }
    {
        vector<string> dlines = readAllLines(doctorDataFile);
        if (drrn < 0 || drrn >= (int)dlines.size() || dlines[drrn].empty() || dlines[drrn][0] == DELETE_FLAG) {
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

    cout << "Appointment updated successfully.\n";
    return true;
}
