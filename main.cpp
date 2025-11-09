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

static string unquote(const string &s){
    string v = trim(s);
    if(!v.empty() && v.front()=='\''){
        if(v.size()>=2 && v.back()=='\'') return v.substr(1, v.size()-2);
        return v.substr(1);
    }
    return v;
}

// Simple query executor for three supported forms
static void executeQuery(const string &query,
                         vector<PrimaryIndex> &doctorPrimary,
                         vector<SecondaryIndex> &doctorSecondary,
                         vector<PrimaryIndex> &apptPrimary,
                         vector<SecondaryIndex> &apptSecondary){
    string q = trim(query);
    if(q.empty()){
        cout << "Empty query\n"; return;
    }
    if(!q.empty() && q.back()==';') q.pop_back();

    // Case-insensitive parse using positions from lower-cased string
    string lq = toLower(q);
    size_t selPos = lq.find("select ");
    size_t fromPos = lq.find(" from ");
    size_t wherePos = lq.find(" where ");
    if(selPos!=0 || fromPos==string::npos || wherePos==string::npos || fromPos<=7 || wherePos<=fromPos+6){
        cout << "Invalid query\n"; return;
    }

    string selectPart = trim(q.substr(7, fromPos-7));
    string tablePart  = trim(q.substr(fromPos+6, wherePos-(fromPos+6)));
    string condPart   = trim(q.substr(wherePos+7));

    string lSelect = toLower(selectPart);
    string lTable  = toLower(tablePart);
    string lCond   = toLower(condPart);

    // condition must be: <field>=<value>
    size_t eqPos = lCond.find('=');
    if(eqPos==string::npos){ cout << "Invalid condition\n"; return; }
    string whereField = trim(condPart.substr(0, eqPos));
    string whereValue = unquote(condPart.substr(eqPos+1));
    string lWhereField = toLower(whereField);

    bool selectAll = (lSelect=="all" || lSelect=="*");

    // Mapping helpers
    auto printDoctorField = [](const Doctor &d, const string &lField){
        if(lField=="doctor id") cout << d.ID << "\n";
        else if(lField=="doctor name") cout << d.Name << "\n";
        else if(lField=="doctor specialty") cout << d.Specialty << "\n";
    };
    auto printAppointmentField = [](const Appointment &a, const string &lField){
        if(lField=="appointment id") cout << a.ID << "\n";
        else if(lField=="doctor id") cout << a.DoctorID << "\n";
        else if(lField=="date") cout << a.Date << "\n";
    };

    if(lTable=="doctors"){
        vector<string> lines = readAllLines(doctorDataFile);

        if(lWhereField=="doctor id"){
            int rrn = getRRNByID(doctorPrimary, whereValue.c_str());
            if(rrn==-1){ cout << "ID not found\n"; return; }
            if(rrn >=0 && rrn < (int)lines.size() && !lines[rrn].empty() && lines[rrn][0] != DELETE_FLAG){
                Doctor d = Doctor::fromLine(lines[rrn]);
                if(selectAll) cout << d; else printDoctorField(d, toLower(selectPart));
            } else cout << "Deleted or not present in file\n";
            return;
        }
        else if(lWhereField=="doctor name"){
            vector<string> ids = getAllIDsByKey(doctorSecondary, whereValue.c_str());
            if(ids.empty()){ cout << "No records with that name\n"; return; }
            for(const auto &id : ids){
                int rrn = getRRNByID(doctorPrimary, id.c_str());
                if(rrn!=-1 && rrn<(int)lines.size() && !lines[rrn].empty() && lines[rrn][0] != DELETE_FLAG){
                    Doctor d = Doctor::fromLine(lines[rrn]);
                    if(selectAll) cout << d; else printDoctorField(d, toLower(selectPart));
                }
            }
            return;
        }
        else if(lWhereField=="doctor specialty"){
            // No index on Specialty: scan file
            int hits=0;
            for(int rrn=0; rrn<(int)lines.size(); ++rrn){
                if(lines[rrn].empty() || lines[rrn][0]==DELETE_FLAG) continue;
                auto fields = split(lines[rrn]);
                if(fields.size()>=3 && fields[1]==whereValue){
                    Doctor d = Doctor::fromLine(lines[rrn]);
                    if(selectAll) cout << d; else printDoctorField(d, toLower(selectPart));
                    hits++;
                }
            }
            if(hits==0) cout << "No records match\n";
            return;
        }
        else{
            cout << "Unsupported Doctors field\n"; return;
        }
    }
    else if(lTable=="appointments"){
        vector<string> lines = readAllLines(appointmentDataFile);

        if(lWhereField=="appointment id"){
            int rrn = getRRNByID(apptPrimary, whereValue.c_str());
            if(rrn==-1){ cout << "ID not found\n"; return; }
            if(rrn>=0 && rrn<(int)lines.size() && !lines[rrn].empty() && lines[rrn][0] != DELETE_FLAG){
                Appointment a = Appointment::fromLine(lines[rrn]);
                if(selectAll) cout << a; else printAppointmentField(a, toLower(selectPart));
            } else cout << "Deleted or not present in file\n";
            return;
        }
        else if(lWhereField=="doctor id"){
            vector<string> apptIDs = getAllIDsByKey(apptSecondary, whereValue.c_str());
            if(apptIDs.empty()){ cout << "No appointments for that doctor\n"; return; }
            int count=0;
            for(const auto &aid : apptIDs){
                int rrn = getRRNByID(apptPrimary, aid.c_str());
                if(rrn!=-1 && rrn<(int)lines.size() && !lines[rrn].empty() && lines[rrn][0] != DELETE_FLAG){
                    Appointment a = Appointment::fromLine(lines[rrn]);
                    if(selectAll) cout << a; else printAppointmentField(a, toLower(selectPart));
                    count++;
                }
            }
            if(count==0) cout << "No appointments found (records may be deleted)\n";
            return;
        }
        else if(lWhereField=="date"){
            // No index on Date: scan file
            int count=0;
            for(int rrn=0; rrn<(int)lines.size(); ++rrn){
                if(lines[rrn].empty() || lines[rrn][0]==DELETE_FLAG) continue;
                auto fields = split(lines[rrn]);
                if(fields.size()>=3 && fields[2]==whereValue){
                    Appointment a = Appointment::fromLine(lines[rrn]);
                    if(selectAll) cout << a; else printAppointmentField(a, toLower(selectPart));
                    count++;
                }
            }
            if(count==0) cout << "No appointments match\n";
            return;
        }
        else{
            cout << "Unsupported Appointments field\n"; return;
        }
    }
    else{
        cout << "Unsupported table\n"; return;
    }
}

int main(){
    ofstream doctorFile(doctorDataFile, ios::app); doctorFile.close();
    ofstream appointmentFile(appointmentDataFile, ios::app); appointmentFile.close();

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
            int rrn = getRRNByID(doctorPrimary, id);
            if(rrn==-1){ cout << "ID not found\n"; }
            else{
                vector<string> lines = readAllLines(doctorDataFile);
                if(rrn < (int)lines.size() && !lines[rrn].empty() && lines[rrn][0] != DELETE_FLAG){
                    Doctor d = Doctor::fromLine(lines[rrn]); cout << d;
                } else { cout << "Deleted or not present in file\n"; }
            }
        } else if(choice == "8"){
            char id[15]; cout << "Enter Appointment ID: "; cin >> id;
            int rrn = getRRNByID(apptPrimary, id);
            if(rrn==-1){ cout << "ID not found\n"; }
            else{
                vector<string> lines = readAllLines(appointmentDataFile);
                if(rrn < (int)lines.size() && !lines[rrn].empty() && lines[rrn][0] != DELETE_FLAG){
                    Appointment a = Appointment::fromLine(lines[rrn]); cout << a;
                } else { cout << "Deleted or not present in file\n"; }
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
