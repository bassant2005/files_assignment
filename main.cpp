/**
 * ===============================================================
 *  File created by: Rawda Raafat
 *  Purpose: Implements the main menu and query execution
 *           for Doctors and Appointments operations.
 *  Student ID: 20231067
 * ===============================================================
 */
#include "operations.cpp"
#include <iostream>
#include <string>
#include <algorithm>
#include <limits>

using namespace std;

static string trim(const string &s){
    size_t a = s.find_first_not_of(" \t\r\n");
    size_t b = s.find_last_not_of(" \t\r\n");
    if(a==string::npos) return "";
    return s.substr(a,b-a+1);
}

static string toLower(string s){
    transform(s.begin(), s.end(), s.begin(), [](unsigned char c){return (char)tolower(c);});
    return s;
}

static string stripQuotes(string s){
    s = trim(s);
    if(!s.empty() && (s.front()=='"' || s.front()=='\'')) s.erase(0,1);
    if(!s.empty() && (s.back()=='"' || s.back()=='\'')) s.pop_back();
    return s;
}

// Simple query executor for three supported forms
static void executeQuery(const string &query,
                         vector<PrimaryIndex> &doctorPrimary,
                         vector<SecondaryIndex> &doctorSecondary,
                         vector<PrimaryIndex> &apptPrimary,
                         vector<SecondaryIndex> &apptSecondary){
    string q = trim(query);
    if(q.empty()){
        cout << "Empty query\n";
        return;
    }

    // Remove trailing semicolon if present
    if(!q.empty() && q.back()==';') q.pop_back();

    string lq = toLower(q);

    // 1) Select all from Doctors where Doctor ID='xxx'
    if(lq.rfind("select all from doctors where doctor id=", 0) == 0){
        size_t pos = lq.find("=");
        if(pos==string::npos){ cout << "Invalid query\n"; return; }
        string value = stripQuotes(q.substr(pos+1));

        long long off = resolveActiveOffsetForID(doctorPrimary, doctorDataFile, value.c_str());
        if(off == -1){ cout << "ID not found\n"; return; }
        string line = readLineAtOffset(doctorDataFile, off);
        if(!line.empty() && line[0] != DELETE_FLAG){
            Doctor d = Doctor::fromLine(line);


            cout << d;
        } else {
            cout << "Deleted or not present in file\n";
        }
        return;
    }

    // 2) Select all from Appointments where Doctor ID='xxx'
    if(lq.rfind("select all from appointments where doctor id=", 0) == 0){
        size_t pos = lq.find("=");
        if(pos==string::npos){ cout << "Invalid query\n"; return; }
        string value = stripQuotes(q.substr(pos+1));

        vector<Appointment> results = searchAppointmentsByDoctorID(value.c_str(), apptPrimary, apptSecondary);
        if(results.empty()){
            cout << "No appointments for that doctor\n"; return;
        }
        for(const auto &a : results){
            cout << a;
        }
        return;
    }

    // 3) Select Doctor Name from Doctors where Doctor Name='xxx'
    if(lq.rfind("select doctor name from doctors where doctor name=", 0) == 0){
        size_t pos = lq.find("=");
        if(pos==string::npos){ cout << "Invalid query\n"; return; }
        string value = stripQuotes(q.substr(pos+1));



        vector<string> ids = getAllIDsByKey(doctorSecondary, value.c_str());
        if(ids.empty()){ cout << "No records with that name\n"; return; }

        for(const auto &id : ids){
            long long off = resolveActiveOffsetForID(doctorPrimary, doctorDataFile, id.c_str());
            if(off != -1){
                string line = readLineAtOffset(doctorDataFile, off);
                if(!line.empty() && line[0] != DELETE_FLAG){
                    Doctor d = Doctor::fromLine(line);
                    if(!d.isEmpty()) cout << d.Name << "\n";
                }
            }
        }
        return;
    }

    cout << "Unsupported query.\n";
}
//// ===================== Main Menu =====================
int main(){
    ofstream doctorFile(doctorDataFile, ios::app); doctorFile.close();
    ofstream appointmentFile(appointmentDataFile, ios::app); appointmentFile.close();
    Build_indexes();

    vector<PrimaryIndex> doctorPrimary = readPrimaryIndex(doctorPrimaryIndexFile);
    vector<SecondaryIndex> doctorSecondary = readSecondaryIndex(doctorSecondaryIndexFile);
    vector<PrimaryIndex> apptPrimary = readPrimaryIndex(appointmentPrimaryIndexFile);
    vector<SecondaryIndex> apptSecondary = readSecondaryIndex(appointmentSecondaryIndexFile);

    vector<int> doctorAvail;
    vector<int> apptAvail;

    while(true){
        cout << "\n=== Main Menu ===\n";
        cout << "1. Add New Doctor\n";
        cout << "2. Add New Appointment\n";
        cout << "3. Update Doctor Name (Doctor ID)\n";
        cout << "4. Update Appointment Date (Appointment ID)\n";
        cout << "5. Delete Appointment (Appointment ID)\n";
        cout << "6. Delete Doctor (Doctor ID)\n";
        cout << "7. Print Doctor Info (Doctor ID)\n";
        cout << "8. Print Appointment Info (Appointment ID)\n";
        cout << "9. Write Query\n";
        cout << "E. Exit\n";
        cout << "Choose: ";
        string choice; cin >> choice;

        if(choice == "1"){
            addDoctor(doctorPrimary, doctorSecondary, doctorAvail);
        } else if(choice == "2"){
            addAppointment(apptPrimary, apptSecondary, apptAvail, doctorPrimary);
        } else if(choice == "3"){
            updateDoctor(doctorPrimary, doctorSecondary);
        } else if(choice == "4"){
            updateAppointment(apptPrimary, apptSecondary, doctorPrimary);
        } else if(choice == "5"){
            char id[15]; cout << "Enter Appointment ID to delete: "; cin >> id;
            bool ok = deleteAppointmentByID(id, apptPrimary, apptSecondary, apptAvail);
            cout << (ok ? "Deleted\n" : "Not found or already deleted\n");
        } else if(choice == "6"){
            char id[15]; cout << "Enter Doctor ID to delete: "; cin >> id;
            bool ok = deleteDoctorByID(id, doctorPrimary, doctorSecondary, doctorAvail,
                                       apptPrimary, apptSecondary, apptAvail);
            cout << (ok ? "Deleted\n" : "Not found or already deleted\n");
        } else if(choice == "7"){
            char id[15]; cout << "Enter Doctor ID: "; cin >> id;
            // First try to find the doctor in the primary index
            bool found = false;
            for (const auto &entry : doctorPrimary) {
                if (strcmp(entry.recordID, id) == 0) {
                    found = true;
                    string line = readLineAtOffset(doctorDataFile, entry.offset);
                    if(!line.empty() && line[0] != DELETE_FLAG) {
                        Doctor d = Doctor::fromLine(line); 
                        cout << d;
                    } else { 
                        cout << "Doctor " << id << " has been deleted\n";
                    }
                    break;
                }
            }
            if (!found) {
                cout << "Doctor " << id << " not found\n";
            }
        } else if(choice == "8"){
            char id[15]; cout << "Enter Appointment ID: "; cin >> id;
            long long offActive = resolveActiveOffsetForID(apptPrimary, appointmentDataFile, id);
            if (offActive != -1) {
                string line = readLineAtOffset(appointmentDataFile, offActive);
                Appointment a = Appointment::fromLine(line);
                cout << a;
            } else {
                long long anyOff = getOffsetByID(apptPrimary, id);
                if (anyOff != -1) {
                    string line = readLineAtOffset(appointmentDataFile, anyOff);
                    if (!line.empty() && line[0] == DELETE_FLAG) {
                        cout << "Appointment " << id << " has been deleted\n";
                    } else {
                        cout << "Appointment " << id << " not found\n";
                    }
                } else {
                    cout << "Appointment " << id << " not found\n";
                }
            }
        } else if(choice == "9"){
            cout << "Enter query: ";
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            string q; getline(cin, q);
            executeQuery(q, doctorPrimary, doctorSecondary, apptPrimary, apptSecondary);
        } else if(choice == "E" || choice == "e"){
            break;
        } else {
            cout << "Invalid choice\n";
        }
    }
    cout << "Goodbye\n";
    return 0;
}
