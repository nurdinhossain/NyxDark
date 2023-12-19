#include "hashtable.h"
#include <cmath>
#include <iostream>

HashTable::HashTable(int tableSize) {
    // ensure size is a power of 2
    int tSize = 1 << (int)log2(tableSize);

    // set size
    size = tSize * 1024 * 1024 / sizeof(Entry);

    // initialize array
    table = new Entry[size];

    // clear the table
    clear();
}

HashTable::~HashTable() {
    // delete the table
    delete[] table;
}

void HashTable::store(UInt64 hash, int value) {
    int index = hash % size;
    Entry& entry = table[index];
    while (entry.hash != 0 && entry.hash != hash) {
        index = (index + 1) % size;
        entry = table[index];
    }
    entry.hash = hash;
    entry.value = value;
}

bool HashTable::lookup(UInt64 hash, int& value) const {
    int index = hash % size;
    const Entry& entry = table[index];
    if (entry.hash == hash) {
        value = entry.value;
        return true;
    } else {
        for (int i = 1; i < size; i++) {
            int probeIndex = (index + i) % size;
            const Entry& probeEntry = table[probeIndex];
            if (probeEntry.hash == hash) {
                value = probeEntry.value;
                return true;
            } else if (probeEntry.hash == 0 || probeIndex == index) {
                break;
            }
        }
        return false;
    }
}

void HashTable::remove(UInt64 hash)
{
    int index = hash % size;
    Entry& entry = table[index];
    if (entry.hash == hash)
    {
        entry.hash = 0;
        entry.value = 0;
    }
    else
    {
        for (int i = 1; i < size; i++)
        {
            int probeIndex = (index + i) % size;
            Entry& probeEntry = table[probeIndex];
            if (probeEntry.hash == hash)
            {
                probeEntry.hash = 0;
                probeEntry.value = 0;
                break;
            }
            else if (probeEntry.hash == 0 || probeIndex == index)
            {
                break;
            }
        }
    }
}

HashTable* HashTable::clone() const
{
    HashTable* newTable = new HashTable(size * sizeof(Entry) / 1024 / 1024);
    for (int i = 0; i < size; i++)
    {
        const Entry& entry = table[i];
        if (entry.hash != 0)
        {
            newTable->store(entry.hash, entry.value);
        }
    }
    return newTable;
}