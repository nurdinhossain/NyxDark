#include "pawnhash.h"
#include <iostream>
#include <cmath>

// constructor
PawnTable::PawnTable()
{
    // ensure size is a power of 2
    int tt_size = 1 << (int)log2(PAWN_HASH_SIZE);

    // set size
    size_ = tt_size * 1024 * 1024 / sizeof(PawnEntry);

    // initialize array
    table_ = new PawnEntry[size_];

    // clear the table
    clear();
}

PawnTable::PawnTable(int mb)
{
    // ensure size is a power of 2
    mb = 1 << (int)log2(mb);

    // set size
    size_ = mb * 1024 * 1024 / sizeof(PawnEntry);

    // initialize array
    table_ = new PawnEntry[size_];

    // clear the table
    clear();
}

// destructor
PawnTable::~PawnTable()
{
    // delete the table
    delete[] table_;
}

// helpers/getters
void PawnTable::clear()
{
    // clear the table
    for (int i = 0; i < size_; i++)
    {
        table_[i].key = 0;
        table_[i].whiteAttacks = 0;
        table_[i].blackAttacks = 0;
        table_[i].openingScore = 0;
        table_[i].endgameScore = 0;
    }
}

// store/access
void PawnTable::store(UInt64 key, UInt64 whiteAttacks, UInt64 blackAttacks, int openingScore, int endgameScore)
{
    // probe the table
    PawnEntry* entry = probe(key);

    // store the data
    entry->key = key;
    entry->whiteAttacks = whiteAttacks;
    entry->blackAttacks = blackAttacks;
    entry->openingScore = openingScore;
    entry->endgameScore = endgameScore;
}

PawnEntry* PawnTable::probe(UInt64 key)
{
    // get the index
    int index = key % size_;

    // return the entry
    return &table_[index];
}