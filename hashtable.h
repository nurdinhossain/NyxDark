#pragma once
using UInt64 = unsigned long long;

class HashTable {
public:
    HashTable(int tableSize); // in MB   
    ~HashTable();
    void store(UInt64 hash, int value);
    bool lookup(UInt64 hash, int& value) const;
    void remove(UInt64 hash);
    void clear() { for (int i = 0; i < size; i++) { table[i].hash = 0; table[i].value = 0; } }
    HashTable* clone() const;
private:
    struct Entry {
        UInt64 hash;
        int value;
    };
    int size;
    Entry* table;
};