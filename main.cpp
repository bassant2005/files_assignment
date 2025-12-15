#include "operation.cpp"

void TestIndexOperations(const char* filename)
{
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

    InsertNewRecordAtIndex((char*)filename, 30, 96);
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

int main()
{
    const char filename[] = "IndexFile.bin";
    int numberOfNodes = 10;

    // Create initial file
    CreateIndexFile((char*)filename, numberOfNodes);
    cout<<"Empty B-Tree file created.\n\n";

    cout<<"Initial B-Tree index file content:\n";
    DisplayIndexFileContent((char*)filename);

    TestIndexOperations(filename);
}
