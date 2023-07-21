#include "transposition.h"
#include "evaluate.h"
#include "evalparams.h"
#include <iostream>
#include <cmath>

// constructor and destructor
TranspositionTable::TranspositionTable()
{
    // ensure size is a power of 2
    int tt_size = 1 << (int)log2(TT_SIZE);

    // set size
    size_ = tt_size * 1024 * 1024 / sizeof(Entry);

    // initialize array
    table_ = new Entry[size_];

    // clear the table
    clear();

    // print size
    std::cout << "Transposition table size: " << size_ << std::endl;
}

TranspositionTable::TranspositionTable(int mb)
{
    // ensure size is a power of 2
    mb = 1 << (int)log2(mb);

    // set size
    size_ = mb * 1024 * 1024 / sizeof(Entry);

    // initialize array
    table_ = new Entry[size_];

    // clear the table
    clear();

    // print size
    std::cout << "Transposition table size: " << size_ << std::endl;
}

TranspositionTable::~TranspositionTable()
{
    // delete the table
    delete[] table_;
}

// helpers/getters 

void TranspositionTable::clear()
{
    // clear the table
    for (int i = 0; i < size_; i++)
    {
        table_[i].key = 0ULL;
        table_[i].move = {QUIET, NONE, NONE};
        table_[i].score = 0;
        table_[i].depth = 0;
        table_[i].flag = NO_FLAG;
    }
}

int TranspositionTable::correctScoreStore(int score, int ply)
{
    // if the score is a mate score, adjust it
    if (score >= MATE - MATE_BUFFER)
    {
        score += ply;
    }
    else if (score <= -MATE + MATE_BUFFER)
    {
        score -= ply;
    }

    // return the score
    return score;
}

int TranspositionTable::correctScoreRead(int score, int ply)
{
    // if the score is a mate score, adjust it
    if (score >= MATE - MATE_BUFFER)
    {
        score -= ply;
    }
    else if (score <= -MATE + MATE_BUFFER)
    {
        score += ply;
    }

    // return the score
    return score;
}

// store/access
Entry* TranspositionTable::probe(UInt64 key)
{
    // get the index
    int index = key % size_;

    // return the entry
    return &table_[index];
}

void TranspositionTable::store(UInt64 key, Flag flag, int depth, int ply, int score, Move move)
{
    // store the data
    Entry* entry = probe(key);
    entry->key = key;
    entry->flag = flag;
    entry->depth = depth;
    entry->score = correctScoreStore(score, ply);
    entry->move = move;
}

// other
void TranspositionTable::display()
{
    // print the table
    for (int i = 0; i < size_; i++)
    {
        std::cout << "Entry " << i << ": " << std::endl;
        std::cout << "Key: " << table_[i].key << std::endl;
        std::cout << "Flag: " << table_[i].flag << std::endl;
        std::cout << "Depth: " << table_[i].depth << std::endl;
        std::cout << "Score: " << table_[i].score << std::endl;
        std::cout << "Move: " << indexToSquare(table_[i].move.from) << indexToSquare(table_[i].move.to) << std::endl;
        std::cout << std::endl;
    }
}
