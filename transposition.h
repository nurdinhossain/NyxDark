#pragma once
#include "game.h"

// constants
const int TT_SIZE = 32; // in MB

// enum for flags
enum Flag
{
    EXACT,
    LOWER_BOUND,
    UPPER_BOUND,
    NO_FLAG
};

// struct for storing transposition table entries
struct Entry
{
    UInt64 key{0ULL};
    Move move{Move()};
    int score{0};
    int depth{0};
    Flag flag{NO_FLAG};
};

// class for transposition table
class TranspositionTable
{
    public:
        // constructor and destructor
        TranspositionTable();
        TranspositionTable(int mb);
        ~TranspositionTable();

        // helpers/getters
        void clear();
        int correctScoreStore(int score, int ply);
        int correctScoreRead(int score, int ply);

        // store/access
        void store(UInt64 key, Flag flag, int depth, int ply, int score, Move move);
        Entry* probe(UInt64 key);
    private:
        // variables
        int size_;
        Entry* table_;
};