// combined_doctors_appointments.cpp
// Single-file program: manages Doctors and Appointments with indices + avail lists (text mode, char arrays)
#include "createfile.cpp"
#include <bits/stdc++.h>
using namespace std;

const char DELETE_FLAG = '*';

// file names
const string DOCTOR_FILE = "Doctors.txt";
const string DOCTOR_PRIMARY_FILE = "DoctorPrimaryIndex.txt";
const string DOCTOR_SECONDARY_FILE = "DoctorSecondaryIndex.txt";

const string APPT_FILE = "Appointments.txt";
const string APPT_PRIMARY_FILE = "AppointmentPrimaryIndex.txt";
const string APPT_SECONDARY_FILE = "AppointmentSecondaryIndex.txt";

// -------------------- Forward Declarations --------------------
struct Doctor;
struct Appointment;

// Function declarations
bool deleteAppointmentByID(const char *id, vector<PrimaryIndex> &primary,
                           vector<SecondaryIndex> &secondary, vector<int> &avail);
bool deleteAllAppointmentsForDoctor(const char *doctorID,
                                    vector<PrimaryIndex> &apptPrimary,
                                    vector<SecondaryIndex> &apptSecondary,
                                    vector<int> &apptAvail);
vector<Appointment> searchAppointmentsByDoctorID(const char *doctorID,
                                                 const vector<PrimaryIndex> &apptPrimary,
                                                 const vector<SecondaryIndex> &apptSecondary);

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

    Doctor(string n, string s, string id) {
        strcpy(Name, n.c_str());
        strcpy(Specialty, s.c_str());
        strcpy(ID, id.c_str());
    }

    static Doctor fromLine(const string &line) {
        stringstream ss(line);
        string name,speciality,id;
        ss >> ws;
        ss >> name >> speciality >> id;
        if(!line.empty() && line[0] == DELETE_FLAG) return Doctor();
        return Doctor(name.c_str(), speciality.c_str(), id.c_str());
    }

    string toLine() const {
        string s = " ";
        s += string(Name) + " " + string(Specialty) + " " + string(ID);
        return s;
    }
    friend ostream& operator<<(ostream &os, const Doctor &d) {
        if(d.Name=="") os << "Deleted or empty record\n";
        else os << "Name: " << d.Name << " | Specialty: " << d.Specialty << " | ID: " << d.ID << "\n";
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

    Appointment(string id, string docID, string date) {
        strcpy(ID, id.c_str());
        strcpy(DoctorID, docID.c_str());
        strcpy(Date, date.c_str());
    }

    static Appointment fromLine(const string &line) {
        if(line.empty() || line[0] == DELETE_FLAG) return Appointment();
        string appid, drid, date;
        stringstream ss(line);
        ss >> ws;
        ss >> appid >> drid >> date;
        return Appointment(appid.c_str(), drid.c_str(), date.c_str());
    }

    string toLine() const {
        return string(" ") + string(ID) + " " + string(DoctorID) + " " + string(Date);
    }

    friend ostream& operator<<(ostream &os, const Appointment &a) {
        if (a.ID[0] == '\0') os << "Deleted or empty record\n";
        else os << "AppointmentID: " << a.ID << " | DoctorID: " << a.DoctorID
                << " | Date: " << a.Date << endl;
        return os;
    }
};

// load primary index from fileName
vector<PrimaryIndex> loadPrimaryIndexFile(const string &fileName) {
    vector<PrimaryIndex> outv;
    ifstream in(fileName);
    string line;
    while(getline(in, line)) {
        if(line.empty()) continue;
        auto pos = line.find('|');
        string id = line.substr(0,pos);
        string rrnStr = line.substr(pos+1);
        PrimaryIndex p;
        strncpy(p.recordID, id.c_str(), 14); p.recordID[14]='\0';
        p.recordRRN = stoi(rrnStr);
        outv.push_back(p);
    }
    sort(outv.begin(), outv.end());
    return outv;
}

// load secondary index from fileName
vector<SecondaryIndex> loadSecondaryIndexFile(const string &fileName) {
    vector<SecondaryIndex> outv;
    ifstream in(fileName);
    string line;
    while(getline(in, line)) {
        if(line.empty()) continue;
        auto pos = line.find('|');
        string key = line.substr(0,pos);
        string id = line.substr(pos+1);
        SecondaryIndex s;
        strncpy(s.keyValue, key.c_str(), 29); s.keyValue[29]='\0';
        strncpy(s.linkedID, id.c_str(), 14); s.linkedID[14]='\0';
        outv.push_back(s);
    }
    sort(outv.begin(), outv.end());
    return outv;
}

// -------------------- Utility search functions --------------------
// binary search primary index array by id (char*)
int getRRNByID(PrimaryIndex arr[], int n, const char *id) {
    int l=0, r=n-1;
    while(l<=r) {
        int m=(l+r)/2;
        int cmp = strcmp(id, arr[m].recordID);
        if(cmp==0) return arr[m].recordRRN;
        else if(cmp < 0) r=m-1;
        else l=m+1;
    }
    return -1;
}

// get all IDs linked to a secondary key (name or doctorID)
vector<string> getAllIDsByKey(SecondaryIndex arr[], int n, const char *key) {
    vector<string> ids;
    if(n==0) return ids;
    int l=0, r=n-1, mid=-1;
    while(l<=r) {
        mid=(l+r)/2;
        int cmp = strcmp(key, arr[mid].keyValue);
        if(cmp==0) break;
        else if(cmp < 0) r = mid-1;
        else l = mid+1;
    }
    if(l>r) return ids; // none found
    // scan left from mid
    int i = mid;
    while(i >= 0 && strcmp(arr[i].keyValue, key) == 0) {
        ids.push_back(string(arr[i].linkedID));
        i--;
    }
    // scan right from mid+1
    i = mid+1;
    while(i < n && strcmp(arr[i].keyValue, key) == 0) {
        ids.push_back(string(arr[i].linkedID));
        i++;
    }
    return ids;
}

// -------------------- File utilities (read/write lines) --------------------
vector<string> readAllLines(const string &fileName) {
    vector<string> lines;
    ifstream in(fileName);
    string line;
    while(getline(in, line)) lines.push_back(line);
    return lines;
}

void writeAllLines(const string &fileName, const vector<string> &lines) {
    ofstream out(fileName);
    for(const auto &l : lines) out << l << "\n";
}

// -------------------- Avail list printer for testing--------------------
void printAvail(const vector<int> &avail) {
    if(avail.empty()) { cout << "Avail list is empty.\n"; return; }
    cout << "Available RRNs: ";
    for(size_t i=0;i<avail.size();++i) {
        cout << (avail[i]+1);
        if(i+1<avail.size()) cout << ", ";
    }
    cout << "\n";
}

// -------------------- NEW FUNCTION: Search appointments by Doctor ID --------------------
vector<Appointment> searchAppointmentsByDoctorID(const char *doctorID,
                                                 const vector<PrimaryIndex> &apptPrimary,
                                                 const vector<SecondaryIndex> &apptSecondary) {
    vector<Appointment> appointments;

    // Get all appointment IDs for this doctor
    vector<string> apptIDs = getAllIDsByKey(const_cast<SecondaryIndex*>(apptSecondary.data()),
                                            (int)apptSecondary.size(), doctorID);

    if(apptIDs.empty()) return appointments;

    // Load appointment file
    vector<string> lines = readAllLines(APPT_FILE);

    // Get appointment details for each ID
    for(auto &apptID : apptIDs) {
        int rrn = getRRNByID(const_cast<PrimaryIndex*>(apptPrimary.data()),
                             (int)apptPrimary.size(), apptID.c_str());
        if(rrn != -1 && rrn < (int)lines.size() && !lines[rrn].empty() && lines[rrn][0] != DELETE_FLAG) {
            appointments.push_back(Appointment::fromLine(lines[rrn]));
        }
    }

    return appointments;
}

// -------------------- NEW FUNCTION: Delete all appointments for a doctor --------------------
bool deleteAllAppointmentsForDoctor(const char *doctorID,
                                    vector<PrimaryIndex> &apptPrimary,
                                    vector<SecondaryIndex> &apptSecondary,
                                    vector<int> &apptAvail) {
    // Get all appointment IDs for this doctor
    vector<string> apptIDs = getAllIDsByKey(apptSecondary.data(),
                                            (int)apptSecondary.size(), doctorID);

    if(apptIDs.empty()) {
        cout << "No appointments found for doctor " << doctorID << endl;
        return true; // No appointments to delete is not an error
    }

    cout << "Found " << apptIDs.size() << " appointment(s) for doctor " << doctorID << ". Deleting...\n";

    bool allDeleted = true;
    int deletedCount = 0;

    // Delete each appointment
    for(auto &apptID : apptIDs) {
        bool success = deleteAppointmentByID(apptID.c_str(), apptPrimary, apptSecondary, apptAvail);
        if(success) {
            deletedCount++;
        } else {
            allDeleted = false;
            cout << "Failed to delete appointment " << apptID << endl;
        }
    }

    cout << "Successfully deleted " << deletedCount << " appointment(s) for doctor " << doctorID << endl;
    return allDeleted;
}

// -------------------- Appointment operations --------------------
bool deleteAppointmentByID(
        const char *id,
        vector<PrimaryIndex> &primary,
        vector<SecondaryIndex> &secondary,
        vector<int> &avail
) {
    int rrn = getRRNByID(primary.data(), (int)primary.size(), id);
    if(rrn == -1) return false;
    vector<string> lines = readAllLines(APPT_FILE);
    if(rrn >= (int)lines.size()) return false;
    if(!lines[rrn].empty() && lines[rrn][0]==DELETE_FLAG) return false;
    lines[rrn][0] = DELETE_FLAG;
    writeAllLines(APPT_FILE, lines);
    primary.erase(remove_if(primary.begin(), primary.end(), [&](const PrimaryIndex &p){ return strcmp(p.recordID, id)==0; }), primary.end());
    secondary.erase(remove_if(secondary.begin(), secondary.end(), [&](const SecondaryIndex &s){ return strcmp(s.linkedID, id)==0; }), secondary.end());
    sort(primary.begin(), primary.end());
    sort(secondary.begin(), secondary.end());
    writePrimaryIndex( primary , APPT_PRIMARY_FILE);
    writeSecondaryIndex( secondary , APPT_SECONDARY_FILE);
    avail.push_back(rrn);
    return true;
}

int insertAppointment(
        const Appointment &a,
        vector<PrimaryIndex> &primary,
        vector<SecondaryIndex> &secondary,
        vector<int> &avail
) {
    vector<string> lines = readAllLines(APPT_FILE);
    int rrn = -1;
    if(!avail.empty()) {
        rrn = avail.back();
        avail.pop_back();
        if(rrn < (int)lines.size()) lines[rrn] = a.toLine();
        else {
            while((int)lines.size() < rrn) lines.push_back(string());
            lines.push_back(a.toLine());
        }
    } else {
        lines.push_back(a.toLine());
        rrn = (int)lines.size()-1;
    }
    writeAllLines(APPT_FILE, lines);

    PrimaryIndex p; strncpy(p.recordID, a.ID, 14); p.recordID[14]='\0'; p.recordRRN = rrn;
    primary.push_back(p);
    SecondaryIndex s; strncpy(s.keyValue, a.DoctorID, 29); s.keyValue[29]='\0'; strncpy(s.linkedID, a.ID, 14); s.linkedID[14]='\0';
    secondary.push_back(s);
    sort(primary.begin(), primary.end());
    sort(secondary.begin(), secondary.end());
    writePrimaryIndex( primary , APPT_PRIMARY_FILE);
    writeSecondaryIndex( secondary , APPT_SECONDARY_FILE);
    return rrn;
}

// -------------------- Doctor operations --------------------
bool insertDoctor(
        const Doctor &doc,
        vector<PrimaryIndex> &primary,
        vector<SecondaryIndex> &secondary,
        vector<int> &avail
) {
    vector<string> lines = readAllLines(DOCTOR_FILE);
    int rrn;

    // reuse from avail list if possible
    if (!avail.empty()) {
        rrn = avail.back();
        avail.pop_back();
        lines[rrn] = doc.toLine();
    } else {
        rrn = lines.size();
        lines.push_back(doc.toLine());
    }

    writeAllLines(DOCTOR_FILE, lines);

    // === ADD TO PRIMARY INDEX ===
    PrimaryIndex p;
    strncpy(p.recordID, doc.ID, 14);
    p.recordID[14] = '\0';
    p.recordRRN = rrn;
    primary.push_back(p);
    sort(primary.begin(), primary.end());
    writePrimaryIndex(primary, DOCTOR_PRIMARY_FILE);

    // === ADD TO SECONDARY INDEX ===
    SecondaryIndex s;
    strncpy(s.keyValue, doc.Name, 29);
    s.keyValue[29] = '\0';
    strncpy(s.linkedID, doc.ID, 14);
    s.linkedID[14] = '\0';
    secondary.push_back(s);
    sort(secondary.begin(), secondary.end());
    writeSecondaryIndex(secondary, DOCTOR_SECONDARY_FILE);

    return true;
}

bool deleteDoctorByID(
        const char *id,
        vector<PrimaryIndex> &primary,
        vector<SecondaryIndex> &secondary,
        vector<int> &avail,
        vector<PrimaryIndex> &apptPrimary,
        vector<SecondaryIndex> &apptSecondary,
        vector<int> &apptAvail
) {
    int rrn = getRRNByID(primary.data(), (int)primary.size(), id);
    if(rrn == -1) return false;

    // First delete all appointments for this doctor
    cout << "Deleting all appointments for doctor " << id << "..." << endl;
    deleteAllAppointmentsForDoctor(id, apptPrimary, apptSecondary, apptAvail);

    // Then delete the doctor
    vector<string> lines = readAllLines(DOCTOR_FILE);
    if(rrn >= (int)lines.size()) return false;
    if(!lines[rrn].empty() && lines[rrn][0]==DELETE_FLAG) return false; // mark deleted
    lines[rrn][0] = DELETE_FLAG;
    writeAllLines(DOCTOR_FILE, lines);
    // remove indices
    primary.erase(remove_if(primary.begin(), primary.end(), [&](const PrimaryIndex &p){ return strcmp(p.recordID, id)==0; }), primary.end());
    secondary.erase(remove_if(secondary.begin(), secondary.end(), [&](const SecondaryIndex &s){ return strcmp(s.linkedID, id)==0; }), secondary.end());
    sort(primary.begin(), primary.end());
    sort(secondary.begin(), secondary.end());
    writePrimaryIndex( primary , DOCTOR_PRIMARY_FILE);
    writeSecondaryIndex( secondary , DOCTOR_SECONDARY_FILE);
    // add to avail
    avail.push_back(rrn);
    return true;
}

// -------------------- Manage Doctors menu --------------------
void manageDoctors(
        vector<PrimaryIndex> &primary,
        vector<SecondaryIndex> &secondary,
        vector<int> &avail,
        vector<PrimaryIndex> &apptPrimary,
        vector<SecondaryIndex> &apptSecondary,
        vector<int> &apptAvail
) {
    char choice;
    do {
        cout << "\n--- Manage Doctors ---\n";
        cout << "I: Insert  V: View by RRN  D: Delete by ID  S: Search by ID  N: Search by Name  A: Print Avail  P: Search Appointments by Doctor ID  B: Back\n";
        cout << "Choose: ";
        cin >> choice;
        if(choice=='I' || choice=='i') {
            Doctor d;
            cout << "Enter Name (no spaces): "; cin >> d.Name;
            cout << "Enter Specialty (no spaces): "; cin >> d.Specialty;
            cout << "Enter ID: "; cin >> d.ID;
            int rrn = insertDoctor(d, primary, secondary, avail);
            cout << "Inserted at RRN: " << rrn+1 << "\n";
        } else if(choice=='V' || choice=='v') {
            int r; cout << "Enter RRN: "; cin >> r; --r;
            vector<string> lines = readAllLines(DOCTOR_FILE);
            if(r < 0 || r >= (int)lines.size()) { cout << "Invalid RRN\n"; continue; }
            if(lines[r].empty() || lines[r][0]==DELETE_FLAG) { cout << "Deleted or empty record\n"; continue; }
            Doctor d = Doctor::fromLine(lines[r]);
            cout << d;
        } else if(choice=='D' || choice=='d') {
            char id[15]; cout << "Enter ID to delete: "; cin >> id;
            bool ok = deleteDoctorByID(id, primary, secondary, avail, apptPrimary, apptSecondary, apptAvail);
            cout << (ok? "Deleted\n" : "Not found or already deleted\n");
        } else if(choice=='S' || choice=='s') {
            char id[15]; cout << "Enter ID: "; cin >> id;
            int rrn = getRRNByID(primary.data(), (int)primary.size(), id);
            if(rrn==-1) cout << "ID not found\n";
            else {
                vector<string> lines = readAllLines(DOCTOR_FILE);
                if(rrn < (int)lines.size() && lines[rrn][0] != DELETE_FLAG) {
                    cout << Doctor::fromLine(lines[rrn]);
                } else cout << "Deleted or not present in file\n";
            }
        } else if(choice=='N' || choice=='n') {
            char name[30]; cout << "Enter Name (exact): "; cin >> ws; cin.getline(name, 30);
            vector<string> ids = getAllIDsByKey(secondary.data(), (int)secondary.size(), name);
            if(ids.empty()) cout << "No records with that name\n";
            else {
                vector<string> lines = readAllLines(DOCTOR_FILE);
                for(auto &id : ids) {
                    int rrn = getRRNByID(primary.data(), (int)primary.size(), id.c_str());
                    if(rrn!=-1 && rrn < (int)lines.size() && lines[rrn][0] != DELETE_FLAG) {
                        cout << Doctor::fromLine(lines[rrn]);
                    }
                }
            }
        } else if(choice=='A' || choice=='a') {
            printAvail(avail);
        } else if(choice=='P' || choice=='p') {
            char doctorID[15]; cout << "Enter Doctor ID to search appointments: "; cin >> doctorID;
            vector<Appointment> appointments = searchAppointmentsByDoctorID(doctorID, apptPrimary, apptSecondary);
            if(appointments.empty()) {
                cout << "No appointments found for doctor " << doctorID << endl;
            } else {
                cout << "Found " << appointments.size() << " appointment(s) for doctor " << doctorID << ":\n";
                for(const auto &appt : appointments) {
                    cout << appt;
                }
            }
        }
    } while(choice != 'B' && choice != 'b');
}

// -------------------- Manage Appointments menu --------------------
void manageAppointments(
        vector<PrimaryIndex> &primary,
        vector<SecondaryIndex> &secondary,
        vector<int> &avail
) {
    char choice;
    do {
        cout << "\n--- Manage Appointments ---\n";
        cout << "I: Insert  V: View by RRN  D: Delete by ID  S: Search by ID  N: Search by DoctorID  A: Print Avail  B: Back\n";
        cout << "Choose: ";
        cin >> choice;
        if(choice=='I' || choice=='i') {
            Appointment a;
            cout << "Enter AppointmentID: "; cin >> a.ID;
            cout << "Enter DoctorID: "; cin >> a.DoctorID;
            cout << "Enter Date (no spaces): "; cin >> a.Date;
            int rrn = insertAppointment(a, primary, secondary, avail);
            cout << "Inserted at RRN: " << rrn+1 << "\n";
        } else if(choice=='V' || choice=='v') {
            int r; cout << "Enter RRN: "; cin >> r; --r;
            vector<string> lines = readAllLines(APPT_FILE);
            if(r < 0 || r >= (int)lines.size()) { cout << "Invalid RRN\n"; continue; }
            if(lines[r].empty() || lines[r][0]==DELETE_FLAG) { cout << "Deleted or empty record\n"; continue; }
            Appointment a = Appointment::fromLine(lines[r]);
            cout << a;
        } else if(choice=='D' || choice=='d') {
            char id[15]; cout << "Enter Appointment ID to delete: "; cin >> id;
            bool ok = deleteAppointmentByID(id, primary, secondary, avail);
            cout << (ok? "Deleted\n" : "Not found or already deleted\n");
        } else if(choice=='S' || choice=='s') {
            char id[15]; cout << "Enter Appointment ID: "; cin >> id;
            int rrn = getRRNByID(primary.data(), (int)primary.size(), id);
            if(rrn==-1) cout << "ID not found\n";
            else {
                vector<string> lines = readAllLines(APPT_FILE);
                if(rrn < (int)lines.size() && lines[rrn][0] != DELETE_FLAG) {
                    cout << Appointment::fromLine(lines[rrn]);
                } else cout << "Deleted or not present in file\n";
            }
        } else if(choice=='N' || choice=='n') {
            char docid[15]; cout << "Enter DoctorID: "; cin >> ws; cin.getline(docid, 15);
            vector<string> ids = getAllIDsByKey(secondary.data(), (int)secondary.size(), docid);
            if(ids.empty()) cout << "No appointments for that doctor\n";
            else {
                vector<string> lines = readAllLines(APPT_FILE);
                for(auto &id : ids) {
                    int rrn = getRRNByID(primary.data(), (int)primary.size(), id.c_str());
                    if(rrn!=-1 && rrn < (int)lines.size() && lines[rrn][0] != DELETE_FLAG) {
                        cout << Appointment::fromLine(lines[rrn]);
                    }
                }
            }
        } else if(choice=='A' || choice=='a') {
            printAvail(avail);
        }
    } while(choice != 'B' && choice != 'b');
}

// -------------------- main --------------------
int main() {
    // Ensure data files exist (create if missing)
    { ofstream out(DOCTOR_FILE, ios::app); }
    { ofstream out(APPT_FILE, ios::app); }

    // load indices (if exist)
    vector<PrimaryIndex> doctorPrimary = loadPrimaryIndexFile(DOCTOR_PRIMARY_FILE);
    vector<SecondaryIndex> doctorSecondary = loadSecondaryIndexFile(DOCTOR_SECONDARY_FILE);
    vector<PrimaryIndex> apptPrimary = loadPrimaryIndexFile(APPT_PRIMARY_FILE);
    vector<SecondaryIndex> apptSecondary = loadSecondaryIndexFile(APPT_SECONDARY_FILE);

    // avail lists (start empty). If you need persistent avail lists, add files for them and load/save similarly.
    vector<int> doctorAvail;
    vector<int> apptAvail;

    char mainChoice;
    do {
        cout << "\n=== Main Menu ===\n";
        cout << "1. Manage Doctors\n";
        cout << "2. Manage Appointments\n";
        cout << "E. Exit\n";
        cout << "Choose: ";
        cin >> mainChoice;

        if(mainChoice == '1') {
            manageDoctors(doctorPrimary, doctorSecondary, doctorAvail, apptPrimary, apptSecondary, apptAvail);
        } else if(mainChoice == '2') {
            manageAppointments(apptPrimary, apptSecondary, apptAvail);
        }
    } while(mainChoice != 'E' && mainChoice != 'e');

    cout << "Goodbye\n";
    return 0;
}
