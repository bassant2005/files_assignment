#include "Btree_deletion.cpp"
#include <limits>

void TestIndexOperations(const char* filename) {
    // ----------- Inserts -----------
    InsertNewRecordAtIndex((char*)filename, 3, 12);
    InsertNewRecordAtIndex((char*)filename, 7, 24);
    InsertNewRecordAtIndex((char*)filename, 10, 48);
    InsertNewRecordAtIndex((char*)filename, 24, 60);
    InsertNewRecordAtIndex((char*)filename, 14, 72);

    cout << "\n--- Display after first inserts ---\n";
    DisplayIndexFileContent((char*)filename);

    InsertNewRecordAtIndex((char*)filename, 19, 84);
    cout << "\n--- Display after inserting (19,84) ---\n";
    DisplayIndexFileContent((char*)filename);

    InsertNewRecordAtIndex((char*)filename, 30, 196);
    InsertNewRecordAtIndex((char*)filename, 15, 108);
    InsertNewRecordAtIndex((char*)filename, 1, 120);
    InsertNewRecordAtIndex((char*)filename, 5, 132);

    cout << "\n--- Display after more inserts ---\n";
    DisplayIndexFileContent((char*)filename);

    InsertNewRecordAtIndex((char*)filename, 2, 144);
    cout << "\n--- Display after inserting (2,144) ---\n";
    DisplayIndexFileContent((char*)filename);

    InsertNewRecordAtIndex((char*)filename, 8, 156);
    InsertNewRecordAtIndex((char*)filename, 9, 168);
    InsertNewRecordAtIndex((char*)filename, 6, 180);
    InsertNewRecordAtIndex((char*)filename, 11, 192);
    InsertNewRecordAtIndex((char*)filename, 12, 204);
    InsertNewRecordAtIndex((char*)filename, 17, 216);
    InsertNewRecordAtIndex((char*)filename, 18, 228);

    cout << "\n--- Display after bulk inserts ---\n";
    DisplayIndexFileContent((char*)filename);

    InsertNewRecordAtIndex((char*)filename, 32, 240);
    cout << "\n--- Display after inserting (32,240) ---\n";
    DisplayIndexFileContent((char*)filename);

    // ----------- Deletes -----------
    DeleteRecordFromIndex((char*)filename, 10);

    cout << "\n--- Display after deleting key 10 ---\n";
    DisplayIndexFileContent((char*)filename);

    DeleteRecordFromIndex((char*)filename, 9);

    cout << "\n--- Display after deleting key 9 ---\n";
    DisplayIndexFileContent((char*)filename);

    DeleteRecordFromIndex((char*)filename, 8);

    cout << "\n--- Display after deleting key 8 ---\n";
    DisplayIndexFileContent((char*)filename);
}
void manualOperations(const char* filename) {
    int choice;
    int recordID, reference;

    while (true) {
        cout << "\n=== Manual B-tree Operations ===\n";
        cout << "1. Insert a record\n";
        cout << "2. Delete a record\n";
        cout << "3. Search for a record\n";
        cout << "4. Display B-tree\n";
        cout << "5. Return to main menu\n";
        cout << "Enter your choice (1-5): ";

        if (!(cin >> choice)) {
            cout << "Invalid input. Please enter a number.\n";
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            continue;
        }

        switch (choice) {
            case 1: // Insert
                cout << "Enter Record ID (integer): ";
                if (!(cin >> recordID)) {
                    cout << "Invalid Record ID.\n";
                    cin.clear();
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    break;
                }
                cout << "Enter Reference (integer): ";
                if (!(cin >> reference)) {
                    cout << "Invalid Reference.\n";
                    cin.clear();
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    break;
                }
                InsertNewRecordAtIndex((char*)filename, recordID, reference);
                cout << "Insert operation completed.\n";
                break;

            case 2: // Delete
                cout << "Enter Record ID to delete: ";
                if (!(cin >> recordID)) {
                    cout << "Invalid Record ID.\n";
                    cin.clear();
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    break;
                }
                DeleteRecordFromIndex((char*)filename, recordID);
                cout << "Delete operation completed.\n";
                break;

            case 3: // Search
                cout << "Enter Record ID to search: ";
                if (!(cin >> recordID)) {
                    cout << "Invalid Record ID.\n";
                    cin.clear();
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    break;
                }
                reference = SearchARecord((char*)filename, recordID);
                if (reference != -1) {
                    cout << "Record found! Reference: " << reference << endl;
                } else {
                    cout << "Record not found.\n";
                }
                break;

            case 4: // Display
                cout << "\n--- Current B-tree Structure ---\n";
                DisplayIndexFileContent((char*)filename);
                break;

            case 5: // Return to main menu
                return;

            default:
                cout << "Invalid choice. Please try again.\n";
        }

        // Clear any remaining input
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
}

int main() {
    const char filename[] = "IndexFile.bin";
    int choice;
    
    // Create initial file
    int numberOfNodes = 10;
    CreateIndexFile((char*)filename, numberOfNodes);
    cout << "Empty B-Tree file created.\n\n";
    
    while (true) {
        cout << "\n=== B-tree Operations Menu ===\n";
        cout << "1. Run Test Operations\n";
        cout << "2. Manual Operations\n";
        cout << "3. Display Current B-tree\n";
        cout << "4. Exit\n";
        cout << "Enter your choice (1-4): ";
        
        if (!(cin >> choice)) {
            cout << "Invalid input. Please enter a number.\n";
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            continue;
        }
        
        switch (choice) {
            case 1: // Run test operations
                cout << "\n=== Running Test Operations ===\n";
                TestIndexOperations(filename);
                break;
                
            case 2: // Manual operations
                manualOperations(filename);
                break;
                
            case 3: // Display current B-tree
                cout << "\n--- Current B-tree Structure ---\n";
                DisplayIndexFileContent((char*)filename);
                break;
                
            case 4: // Exit
                cout << "Exiting program.\n";
                return 0;
                
            default:
                cout << "Invalid choice. Please try again.\n";
        }
        
        // Clear any remaining input
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
    
    return 0;
}
