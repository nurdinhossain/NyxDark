#pragma once 

// params

// inf/mate/draw
extern const int POS_INF;
extern const int NEG_INF;
extern const int FAIL_SCORE;
extern const int MATE;
extern const int DRAW;
extern const int MATE_BUFFER;

// pawn values
extern int PASSED_PAWN;
extern int UNOBSTRUCTED_PASSER;
extern int UNSTOPPABLE_PASSED_PAWN;
extern int CANDIDATE_PASSED_PAWN;
extern int UNOBSTRUCTED_CANDIDATE;
extern int BACKWARD_PAWN_PENALTY;
extern int ISOLATED_PAWN_PENALTY;

// knight values
extern int KNIGHT_OUTPOST;
extern int KNIGHT_OUTPOST_ON_HOLE;
extern int KNIGHT_MOBILITY_MULTIPLIER;
extern int KNIGHT_MOBILITY_OFFSET;
extern int KNIGHT_MOBILITY_TABLE[9];

// bishop values
extern int BISHOP_PAIR;
extern int BISHOP_MOBILITY_MULTIPLIER;
extern int BISHOP_MOBILITY_OFFSET;
extern int BISHOP_MOBILITY_TABLE[14];

// rook values
extern int ROOK_OPEN_FILE;
extern int ROOK_HORIZONTAL_MOBILITY_MULTIPLIER;
extern int ROOK_HORIZONTAL_MOBILITY_OFFSET;
extern int ROOK_HORIZONTAL_MOBILITY_TABLE[8];
extern int ROOK_VERTICAL_MOBILITY_MULTIPLIER;
extern int ROOK_VERTICAL_MOBILITY_OFFSET;
extern int ROOK_VERTICAL_MOBILITY_TABLE[8];

extern int ROOK_CONNECTED;

// king values
extern int KING_BLOCK_ROOK_PENALTY;
extern int SAFETY_TABLE_MULTIPLIER;
extern int SAFETY_TABLE[100];
extern int MINOR_ATTACK_UNITS;
extern int ROOK_ATTACK_UNITS;
extern int QUEEN_ATTACK_UNITS;
extern int ROOK_CHECK_UNITS;
extern int QUEEN_CHECK_UNITS;
extern int PAWN_SHIELD;
extern int PAWN_STORM;
extern int PAWN_SHIELD_DIVISOR;
extern int PAWN_STORM_DIVISOR;