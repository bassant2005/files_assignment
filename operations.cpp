//
// Created by habeb on 11/1/2025.
//
#include "creating_data_files.cpp"
//#include "createfile.cpp"
#include <iostream>
using namespace std;

// ======================== Structures ========================

// Doctor structure
struct Doctor {
    char ID[15];
    char Name[30];
    char Specialty[30];
};

// Appointment structure
struct Appointment {
    char ID[15];
    char DoctorID[30];
    char Date[15];  // Format: YYYY-MM-DD
};

// Primary index structure
struct PIndex {
    int RRN;
    char ID[15];
};

// Secondary index structure
struct SIndex {
    char Key[30];  // For doctor: Name or DoctorID for appointment
    char ID[10];
};

// ======================== Read Primary Index ========================
void ReadPrimIndex(PIndex PrmIndxArray[], int numRec, fstream& inFile) {
    string line;
    for (int i = 0; i < numRec && getline(inFile, line); i++) {
        size_t pos = line.find('|');
        if (pos != string::npos) {
            string id = line.substr(0, pos); //extracts ID
            string rrn_str = line.substr(pos + 1); //extracts RRN
            strcpy(PrmIndxArray[i].ID, id.c_str());
            PrmIndxArray[i].RRN = stoi(rrn_str); //converts RRN string to integer
        }
    }
}

// ======================== Binary Search on Primary Index ========================
int GetRecordRRN(PIndex PrmIndxArray[], int numRec, string ID) {
    int RRN = -1;
    int low = 0, mid, high = numRec - 1;
    while (low <= high) {
        mid = (low + high) / 2;
        if (ID < PrmIndxArray[mid].ID)
            high = mid - 1;
        else if (ID > PrmIndxArray[mid].ID)
            low = mid + 1;
        else {
            RRN = PrmIndxArray[mid].RRN;
            break;
        }
    }
    return RRN;
}

// ======================== Retrieve Doctor by RRN ========================
Doctor GetDoctor(int RRN, fstream &infile) {
    Doctor doctor;
    infile.clear();
    infile.seekg(0, ios::beg);
    string line;
    int current = 0;

    while (getline(infile, line)) {
        if (current == RRN - 1) {  // RRN starts from 1
            size_t pos1 = line.find('|');
            size_t pos2 = line.find('|', pos1 + 1);
            size_t pos3 = line.find('|', pos2 + 1);

            strcpy(doctor.ID, line.substr(pos1 + 1, pos2 - pos1 - 1).c_str());
            strcpy(doctor.Name, line.substr(pos2 + 1, pos3 - pos2 - 1).c_str());
            strcpy(doctor.Specialty, line.substr(pos3 + 1).c_str());
            break;
        }
        current++;
    }
    return doctor;
}

// ======================== Retrieve Appointment by RRN ========================
Appointment GetAppionment(int RRN, fstream &infile) {
    Appointment appointment;
    infile.clear();
    infile.seekg(0, ios::beg);
    string line;
    int current = 0;

    while (getline(infile, line)) {
        if (current == RRN - 1) {
            size_t pos1 = line.find('|');
            size_t pos2 = line.find('|', pos1 + 1);
            size_t pos3 = line.find('|', pos2 + 1);

            strcpy(appointment.ID, line.substr(pos1 + 1, pos2 - pos1 - 1).c_str());
            strcpy(appointment.DoctorID, line.substr(pos2 + 1, pos3 - pos2 - 1).c_str());
            strcpy(appointment.Date, line.substr(pos3 + 1).c_str());
            break;
        }
        current++;
    }
    return appointment;
}

// ======================== Search Doctor by ID ========================
void SearchDoctorById(int numRec) {
    fstream file("Doctors.txt", ios::in);
    fstream PrimIndex("DoctorPrimaryIndex.txt", ios::in);

    PIndex *PrmIndxArray = new PIndex[numRec];
    ReadPrimIndex(PrmIndxArray, numRec, PrimIndex);

    char ID[15];
    cout << "Enter Doctor ID: ";
    cin >> ID;

    int RRN = GetRecordRRN(PrmIndxArray, numRec, ID);
    if (RRN == -1)
        cout << "Doctor ID not found.\n";
    else {
        Doctor doctor = GetDoctor(RRN, file);
        cout << "Doctor ID: " << doctor.ID << "  Name: " << doctor.Name
             << "  Specialty: " << doctor.Specialty << endl;
    }

    delete[] PrmIndxArray;
    file.close();
    PrimIndex.close();
}

// ======================== Search Doctor by Name (Secondary Index) ========================
void SearchDoctorByName(int numRec) {
    fstream file("Doctors.txt", ios::in);
    fstream SecIndex("DoctorSecondaryIndex.txt", ios::in);
    fstream PrimIndexFile("DoctorPrimaryIndex.txt", ios::in);

    SIndex *secArray = new SIndex[numRec];
    string line;
    for (int i = 0; i < numRec; i++) {
        getline(SecIndex, line);
        size_t pos = line.find('|');
        strcpy(secArray[i].Key, line.substr(0, pos).c_str());
        strcpy(secArray[i].ID, line.substr(pos + 1).c_str());
    }

    PIndex *primArray = new PIndex[numRec];
    ReadPrimIndex(primArray, numRec, PrimIndexFile);

    char targetName[30];
    cout << "Enter Doctor Name: ";
    cin >> targetName;

    bool found = false;
    for (int i = 0; i < numRec; i++) {
        if (strcmp(secArray[i].Key, targetName) == 0) {
            int RRN = GetRecordRRN(primArray, numRec, secArray[i].ID);
            Doctor doc = GetDoctor(RRN, file);
            cout << "Doctor ID: " << doc.ID << "  Name: " << doc.Name
                 << "  Specialty: " << doc.Specialty << endl;
            found = true;
        }
    }
    if (!found){
        cout << "Doctor name not found.\n";
    }

    delete[] secArray;
    delete[] primArray;
    file.close();
    SecIndex.close();
    PrimIndexFile.close();
}

// ======================== Search Appointment by ID ========================
void SearchAppionmentById(int numRec) {
    fstream file("Appointments.txt", ios::in);
    fstream PrimIndex("AppointmentPrimaryIndex.txt", ios::in);

    PIndex *PrmIndxArray = new PIndex[numRec];
    ReadPrimIndex(PrmIndxArray, numRec, PrimIndex);

    char ID[10];
    cout << "Enter Appointment ID: ";
    cin >> ID;

    int RRN = GetRecordRRN(PrmIndxArray, numRec, ID);
    if (RRN == -1)
        cout << "Appointment ID not found.\n";
    else {
        Appointment app = GetAppionment(RRN, file);
        cout << "Appointment -> ID: " << app.ID << "  Doctor ID: " << app.DoctorID
             << "  Date: " << app.Date << endl;
    }

    delete[] PrmIndxArray;
    file.close();
    PrimIndex.close();
}

// ======================== Search Appointments by Doctor ID (Secondary Index) ========================
void SearchAppionmentByDoctorId(int numRec) {
    fstream file("Appointments.txt", ios::in);
    fstream SecIndex("AppointmentSecondaryIndex.txt", ios::in);
    fstream PrimIndexFile("AppointmentPrimaryIndex.txt", ios::in);

    // Read secondary index
    SIndex *secArray = new SIndex[numRec];
    string line;
    for (int i = 0; i < numRec; i++) {
        getline(SecIndex, line);
        size_t pos = line.find('|');
        strcpy(secArray[i].Key, line.substr(0, pos).c_str());
        strcpy(secArray[i].ID, line.substr(pos + 1).c_str());
    }

    // Read primary index
    PIndex *primArray = new PIndex[numRec];
    ReadPrimIndex(primArray, numRec, PrimIndexFile);

    char targetDoctor[30];
    cout << "Enter Doctor ID: ";
    cin >> targetDoctor;

    bool found = false;
    for (int i = 0; i < numRec; i++) {
        if (strcmp(secArray[i].Key, targetDoctor) == 0) {
            int RRN = GetRecordRRN(primArray, numRec, secArray[i].ID);
            Appointment app = GetAppionment(RRN, file);
            cout << "Appointment -> ID: " << app.ID << "  Doctor ID: " << app.DoctorID
                 << "  Date: " << app.Date << endl;
            found = true;
        }
    }
    if (!found) cout << "No appointments found for this doctor.\n";

    delete[] secArray;
    delete[] primArray;
    file.close();
    SecIndex.close();
    PrimIndexFile.close();
}

// ======================== Create Sample Doctors ========================
void createTestDoctors() {
    fstream file("Doctors.txt", ios::out);

    Doctor doctors[3] = {
            {"D001", "Ali", "Cardiology"},
            {"D002", "Mona", "Neurology"},
            {"D003", "Omar", "Pediatrics"}
    };

    for (auto &d : doctors) {
        string record = "20|" + string(d.ID) + "|" + string(d.Name) + "|" + string(d.Specialty) + "\n";
        file << record;
    }

    file.close();
    cout << "Sample Doctors.txt file created successfully.\n";
}

// ======================== Main Function ========================
int main() {
    cout << "Building all indexes...\n";
    buildAllIndexes();  // Build index files
    cout << "Indexes built.\n";

    createTestDoctors();  // Create test doctor records

    cout << "\nTesting search functions...\n";

    // Example searches (adjust numRec according to records in files)
    SearchDoctorById(3);
    SearchDoctorByName(3);
    SearchAppionmentById(12);
    SearchAppionmentByDoctorId(12);

    cout << "Done!\n";
    return 0;
}
