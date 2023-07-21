#pragma once
using UInt64 = unsigned long long;

// constants
const int PAWN_HASH_SIZE = 16; // in MB

// struct for storing pawn hash entries
struct PawnEntry 
{
    UInt64 key{0ULL};
    UInt64 whiteAttacks{0ULL};
    UInt64 blackAttacks{0ULL};
    int openingScore{0};
    int endgameScore{0};
};

// class for transposition table
class PawnTable
{
    public:
        // constructor and destructor
        PawnTable();
        PawnTable(int mb);
        ~PawnTable();

        // helpers/getters
        void clear();

        // store/access
        void store(UInt64 key, UInt64 whiteAttacks, UInt64 blackAttacks, int openingScore, int endgameScore);
        PawnEntry* probe(UInt64 key);
    private:
        // variables
        int size_;
        PawnEntry* table_;
};