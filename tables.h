#pragma once

// piece-square tables
extern int PAWN_TABLE_OPENING[64];
extern int PAWN_TABLE_ENDGAME[64];
extern int KNIGHT_TABLE_OPENING[64];
extern int KNIGHT_TABLE_ENDGAME[64];
extern int BISHOP_TABLE_OPENING[64];
extern int BISHOP_TABLE_ENDGAME[64];
extern int ROOK_TABLE_OPENING[64];
extern int ROOK_TABLE_ENDGAME[64];
extern int QUEEN_TABLE_OPENING[64];
extern int QUEEN_TABLE_ENDGAME[64];
extern int KING_TABLE_OPENING[64];
extern int KING_TABLE_ENDGAME[64];

// multi-dimensional array of piece-square tables
extern int* TABLES[6][2];