#pragma once
#include "game.h"
#include "pawnhash.h"

// masks
constexpr UInt64 SIDES[2] = { 0xFFFFFFFF, 0xFFFFFFFF00000000 };
constexpr UInt64 NEIGHBORING_FILES[8] = {
    0x303030303030303, 0x707070707070707, 0xE0E0E0E0E0E0E0E, 0x1C1C1C1C1C1C1C1C, 0x3838383838383838, 0x7070707070707070, 0xE0E0E0E0E0E0E0E0, 0xC0C0C0C0C0C0C0C0
};
constexpr UInt64 SQUARES_ABOVE_WHITE_PAWNS[8] = {
    0xFFFFFFFFFFFFFF00, 0xFFFFFFFFFFFF0000, 0xFFFFFFFFFF000000, 0xFFFFFFFF00000000, 0xFFFFFF0000000000, 0xFFFF000000000000, 0xFF00000000000000, 0x0
};
constexpr UInt64 SQUARES_BELOW_BLACK_PAWNS[8] = {
    0x0, 0xFF, 0xFFFF, 0xFFFFFF, 0xFFFFFFFF, 0xFFFFFFFFFF, 0xFFFFFFFFFFFF, 0xFFFFFFFFFFFFFF
};

// lazy eval function
int lazyEvaluate(Board& board);

// full eval function
int evaluate(Board& board, PawnTable* pawnTable);

// bishop stuff
bool hasBishopPair(Board& board, Color color);
int bishopMobility(Board& board, Color color, UInt64 enemyPawnAttacks, int& attackUnits, UInt64 kingSafetyArea);

// knight stuff
bool isHole(Board& board, Color color, Square square);
bool isKnightOutpost(Board& board, Color color, Square square);
int knightScore(Board& board, Color color, UInt64 enemyPawnAttacks, int& attackUnits, UInt64 kingSafetyArea);

// rook stuff
bool openFile(Board& board, Color color, int file);
bool halfOpenFile(Board& board, Color color, int file);
bool kingBlockRook(Board& board, Color color, Square rookSquare);
void rookScore(Board& board, Color color, UInt64 enemyPawnAttacks, int& openingScore, int& endgameScore, int& attackUnits, UInt64 kingSafetyArea);

// pawn stuff
bool isPassed(Board& board, Color color, Square square);
bool isUnstoppable(Board& board, Color color, Square square);
bool isCandidate(Board& board, Color color, Square square);
bool isObstructed(Board& board, Color color, Square square);
bool isProtected(Board& board, Color color, Square square);
bool isIsolated(Board& board, Color color, Square square);
bool isBackward(Board& board, Color color, Square square);
void pawnScore(Board& board, Color color, UInt64& attacks, int& openingScore, int& endgameScore);

// king stuff
UInt64 kingSafetyArea(Color color, Square square);
UInt64 kingDangerArea(Color color, Square square);
int kingPawnShieldScore(Board& board, UInt64 safetyArea, Color color);
int kingPawnStormScore(Board& board, UInt64 dangerArea, Color color);
