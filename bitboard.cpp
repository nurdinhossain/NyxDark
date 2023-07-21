#include "bitboard.h"
#include <iostream>
#include <fstream>

using namespace std;

// attack tables
UInt64 ROOK_MAGIC_ATTACK_TABLE[64][16384];
UInt64 BISHOP_MAGIC_ATTACK_TABLE[64][2048];

// get the least significant bit
int lsb(UInt64 bb)
{
    return __builtin_ctzll(bb);
}

// get the most significant bit
int msb(UInt64 bb)
{
    return 63 - __builtin_clzll(bb);
}

// get the number of bits set to 1
int popCount(UInt64 bb)
{
    return __builtin_popcountll(bb);
}

// display function
void display(UInt64 bb)
{
    for (int i = 7; i >= 0; i--) {
        for (int j = 7; j >= 0; j--) {
            if (bb & (1ULL << (i * 8 + j))) {
                cout << "1 ";
            }
            else {
                cout << "0 ";
            }
        }
        cout << endl;
    }
    cout << endl;
}

// function for processing files for ROOK_MAGIC_ATTACK_TABLE attack masks
void processRookAttacks(string fileName)
{
    // print a progress bar
    cout << "Processing rook attacks... ";

    // file will be a list of numbers separated by commas. sequentially put them into the ROOK_MAGIC_ATTACK_TABLE[64][16384] array
    // there is only 1 line in the file, so just read it and put it into the array (once one dimension is filled, move to the next)
    
    // get line 
    ifstream file(fileName);
    string line;
    getline(file, line);

    // some variables for the loop
    string::size_type sz;
    int index = 0;
    int subIndex = 0;
    string number;
    for (int i = 0; i < line.length(); i++) {
        if (line[i] == ',') {
            // convert string to int
            ROOK_MAGIC_ATTACK_TABLE[index][subIndex] = stoull(number, &sz);

            // reset number
            number = "";

            // increment subIndex
            subIndex++;

            // if subIndex is 16384, increment index and reset subIndex
            if (subIndex == 16384) {
                index++;
                subIndex = 0;
            }
        }
        else {
            number += line[i];
        }
    }

    file.close();

    // print a progress bar
    cout << "Done." << endl;
}

// function for processing files for BISHOP_MAGIC_ATTACK_TABLE attack masks
void processBishopAttacks(string fileName)
{
    // print a progress bar
    cout << "Processing bishop attacks... ";

    // file will be a list of numbers separated by commas. sequentially put them into the BISHOP_MAGIC_ATTACK_TABLE[64][2048] array
    // there is only 1 line in the file, so just read it and put it into the array (once one dimension is filled, move to the next)

    // get line 
    ifstream file(fileName);
    string line;
    getline(file, line);

    // some variables for the loop
    string::size_type sz;
    int index = 0;
    int subIndex = 0;
    string number;
    for (int i = 0; i < line.length(); i++) {
        if (line[i] == ',') {
            // convert string to int
            BISHOP_MAGIC_ATTACK_TABLE[index][subIndex] = stoull(number, &sz);

            // reset number
            number = "";

            // increment subIndex
            subIndex++;

            // if subIndex is 2048, increment index and reset subIndex
            if (subIndex == 2048) {
                index++;
                subIndex = 0;
            }
        }
        else {
            number += line[i];
        }
    }

    file.close();

    // print a progress bar
    cout << "Done." << endl;
}

// get magic index
UInt64 getMagicIndex(UInt64 blockers, UInt64 magic, int bits)
{
    return (blockers * magic) >> (64 - bits);
}

// lookup rook attacks 
UInt64 lookupRookAttack(int square, UInt64 blockers)
{
    UInt64 magic = ROOK_MAGIC_NUMBERS[square];
    UInt64 index = getMagicIndex(blockers & ISOLATE_ROOK_MASKS[square], magic, ROOK_INDEX_LENGTH);

    return ROOK_MAGIC_ATTACK_TABLE[square][index];
}

// lookup bishop attacks
UInt64 lookupBishopAttack(int square, UInt64 blockers)
{
    UInt64 magic = BISHOP_MAGIC_NUMBERS[square];
    UInt64 index = getMagicIndex(blockers & ISOLATE_BISHOP_MASKS[square], magic, BISHOP_INDEX_LENGTH);

    return BISHOP_MAGIC_ATTACK_TABLE[square][index];
}

// lookup queen attacks
UInt64 lookupQueenAttack(int square, UInt64 blockers)
{
    return lookupRookAttack(square, blockers) | lookupBishopAttack(square, blockers);
}



