#include "evaluate.h"
#include "bitboard.h"
#include "evalparams.h"
#include <iostream>
#include <cmath>

// lazy evaluation
int lazyEvaluate(Board& board)
{
    int phase = board.getPhase();

    // material
    int openingScore = board.getMaterial(WHITE) - board.getMaterial(BLACK);

    // ensure endgame score equals opening score for material
    int endgameScore = openingScore;

    // piece square tables
    openingScore += board.getStaticEvalOpening();
    endgameScore += board.getStaticEvalEndgame();

    // interpolate between opening and endgame
    int score = (openingScore * (256 - phase) + endgameScore * phase) / 256;

    // return the score based on the side to move
    return score * (board.getNextMove() == WHITE ? 1 : -1);
}

// eval function
int evaluate(Board& board, PawnTable* pawnTable)
{
    int phase = board.getPhase();

    // material
    int openingScore = board.getMaterial(WHITE) - board.getMaterial(BLACK);

    // ensure endgame score equals opening score for material
    int endgameScore = openingScore;

    // king safety stuff
    Square whiteKingIndex = static_cast<Square>(lsb(board.getPiece(WHITE, KING))), blackKingIndex = static_cast<Square>(lsb(board.getPiece(BLACK, KING)));
    
    // check for king pawn shield scaled by enemy material
    UInt64 whiteSafetyArea = kingSafetyArea(WHITE, whiteKingIndex), blackSafetyArea = kingSafetyArea(BLACK, blackKingIndex);
    openingScore += kingPawnShieldScore(board, whiteSafetyArea, WHITE) * PAWN_SHIELD * board.getMaterial(BLACK) / std::max(1, PAWN_SHIELD_DIVISOR * 100);
    openingScore -= kingPawnShieldScore(board, blackSafetyArea, BLACK) * PAWN_SHIELD * board.getMaterial(WHITE) / std::max(1, PAWN_SHIELD_DIVISOR * 100);

    // penalty for being stormed by enemy pawns
    UInt64 whiteDangerArea = kingDangerArea(BLACK, whiteKingIndex), blackDangerArea = kingDangerArea(WHITE, blackKingIndex);
    openingScore -= kingPawnStormScore(board, whiteDangerArea, WHITE) * PAWN_STORM * board.getMaterial(BLACK) / std::max(1, PAWN_STORM_DIVISOR * 100);
    openingScore += kingPawnStormScore(board, blackDangerArea, BLACK) * PAWN_STORM * board.getMaterial(WHITE) / std::max(1, PAWN_STORM_DIVISOR * 100);

    // attack units for safety area
    int whiteAttackUnits = 0, blackAttackUnits = 0;
    whiteSafetyArea |= KING_ATTACKS[whiteKingIndex];
    blackSafetyArea |= KING_ATTACKS[blackKingIndex];

    // attack units for queen
    UInt64 whiteQueens = board.getPiece(WHITE, QUEEN), blackQueens = board.getPiece(BLACK, QUEEN);
    while (whiteQueens)
    {
        Square index = static_cast<Square>(lsb(whiteQueens));
        UInt64 attack = lookupQueenAttack(index, board.getFullOccupied() ^ (1ULL << index));
        whiteAttackUnits += popCount(attack & blackSafetyArea) * QUEEN_ATTACK_UNITS;
        if (attack & board.getPiece(BLACK, KING)) whiteAttackUnits += QUEEN_CHECK_UNITS;
        whiteQueens &= whiteQueens - 1;
    }
    while (blackQueens)
    {
        Square index = static_cast<Square>(lsb(blackQueens));
        UInt64 attack = lookupQueenAttack(index, board.getFullOccupied() ^ (1ULL << index));
        blackAttackUnits += popCount(attack & whiteSafetyArea) * QUEEN_ATTACK_UNITS;
        if (attack & board.getPiece(WHITE, KING)) blackAttackUnits += QUEEN_CHECK_UNITS;
        blackQueens &= blackQueens - 1;
    }

    // bishop pair
    if (hasBishopPair(board, WHITE))
        endgameScore += BISHOP_PAIR;
    if (hasBishopPair(board, BLACK))
        endgameScore -= BISHOP_PAIR;

    // pawn score
    UInt64 whitePawnAttacks = 0ULL, blackPawnAttacks = 0ULL;
    
    // check pawn table for pawn score
    PawnEntry* entry = pawnTable->probe(board.getPawnHash());
    if (entry->key == board.getPawnHash())
    {
        openingScore += entry->openingScore;
        endgameScore += entry->endgameScore;
        whitePawnAttacks = entry->whiteAttacks;
        blackPawnAttacks = entry->blackAttacks;
    }
    else
    {
        int whiteOpeningPawnScore = 0, whiteEndgamePawnScore = 0;
        int blackOpeningPawnScore = 0, blackEndgamePawnScore = 0;
        pawnScore(board, WHITE, whitePawnAttacks, whiteOpeningPawnScore, whiteEndgamePawnScore);
        pawnScore(board, BLACK, blackPawnAttacks, blackOpeningPawnScore, blackEndgamePawnScore);

        // store pawn score in pawn table
        int openingScoreContrib = whiteOpeningPawnScore - blackOpeningPawnScore;
        int endgameScoreContrib = whiteEndgamePawnScore - blackEndgamePawnScore;
        pawnTable->store(board.getPawnHash(), whitePawnAttacks, blackPawnAttacks, openingScoreContrib, endgameScoreContrib);

        openingScore += openingScoreContrib;
        endgameScore += endgameScoreContrib;
    }

    // knight score
    openingScore += knightScore(board, WHITE, blackPawnAttacks, whiteAttackUnits, blackSafetyArea) - knightScore(board, BLACK, whitePawnAttacks, blackAttackUnits, whiteSafetyArea);

    // bishop mobility (square root of number of squares attacked)
    int whiteBishopMobility = (int)(BISHOP_MOBILITY_MULTIPLIER * sqrt((double)bishopMobility(board, WHITE, blackPawnAttacks, whiteAttackUnits, blackSafetyArea))) - BISHOP_MOBILITY_OFFSET;
    int blackBishopMobility = (int)(BISHOP_MOBILITY_MULTIPLIER * sqrt((double)bishopMobility(board, BLACK, whitePawnAttacks, blackAttackUnits, whiteSafetyArea))) - BISHOP_MOBILITY_OFFSET;
    //int whiteBishopMobility = BISHOP_MOBILITY_TABLE[bishopMobility(board, WHITE, blackPawnAttacks, whiteAttackUnits, blackSafetyArea)];
    //int blackBishopMobility = BISHOP_MOBILITY_TABLE[bishopMobility(board, BLACK, whitePawnAttacks, blackAttackUnits, whiteSafetyArea)];
    openingScore += whiteBishopMobility - blackBishopMobility;

    // rook score
    int whiteOpeningRookScore = 0, whiteEndgameRookScore = 0;
    int blackOpeningRookScore = 0, blackEndgameRookScore = 0;
    rookScore(board, WHITE, blackPawnAttacks, whiteOpeningRookScore, whiteEndgameRookScore, whiteAttackUnits, blackSafetyArea);
    rookScore(board, BLACK, whitePawnAttacks, blackOpeningRookScore, blackEndgameRookScore, blackAttackUnits, whiteSafetyArea);
    openingScore += whiteOpeningRookScore - blackOpeningRookScore;
    endgameScore += whiteEndgameRookScore - blackEndgameRookScore;

    // king score
    openingScore += SAFETY_TABLE[whiteAttackUnits] - SAFETY_TABLE[blackAttackUnits];

    // piece square tables
    openingScore += board.getStaticEvalOpening();
    endgameScore += board.getStaticEvalEndgame();

    // interpolate between opening and endgame
    int score = (openingScore * (256 - phase) + endgameScore * phase) / 256;

    // return the score based on the side to move
    return score * (board.getNextMove() == WHITE ? 1 : -1);
}

// bishop 
bool hasBishopPair(Board& board, Color color)
{
    return board.getPieceCount(color, BISHOP) >= 2;
}

int bishopMobility(Board& board, Color color, UInt64 enemyPawnAttacks, int& attackUnits, UInt64 safetyArea)
{
    int score = 0;

    // loop through all bishops
    UInt64 bishops = board.getPiece(color, BISHOP);
    while (bishops)
    {
        Square square = static_cast<Square>(lsb(bishops));
        UInt64 attack = lookupBishopAttack(square, board.getFullOccupied() ^ (1ULL << square));
        score += popCount(attack & ~(enemyPawnAttacks | board.getOccupied(color)));
        attackUnits += popCount(attack & safetyArea) * MINOR_ATTACK_UNITS;
        bishops &= bishops - 1;
    }

    return score;
}

// knight
bool isHole(Board& board, Color color, Square square)
{
    // get frontspan of knight 
    int file = square % 8, rank = square / 8;
    UInt64 frontSpan = color == WHITE ? SQUARES_ABOVE_WHITE_PAWNS[rank] : SQUARES_BELOW_BLACK_PAWNS[rank];
    frontSpan &= (NEIGHBORING_FILES[file] ^ FILE_MASKS[file]);

    // if knight can't ever be attacked by a pawn, return true
    UInt64 enemyPawns = board.getPiece(static_cast<Color>(1-color), PAWN);
    if (!(frontSpan & enemyPawns))
        return true;

    return false;
}

bool isKnightOutpost(Board& board, Color color, Square square)
{
    // get pawn attacks of opposte color on this square and check if it is defended by same color pawn
    UInt64 enemyPawnAttack = PAWN_ATTACKS[1 - color][square];

    // check if there is a pawn on the square and knight is on enemy side
    return (enemyPawnAttack & board.getPiece(color, PAWN)) && (SIDES[1-color] & (1ULL << square));
}

int knightScore(Board& board, Color color, UInt64 enemyPawnAttacks, int& attackUnits, UInt64 safetyArea)
{
    int score = 0;

    // loop through all knights
    UInt64 knights = board.getPiece(color, KNIGHT);
    while (knights)
    {
        Square square = static_cast<Square>(lsb(knights));

        // add bonus for knight outpost
        if (isKnightOutpost(board, color, square))
        {
            score += KNIGHT_OUTPOST;

            if (isHole(board, color, square))
                score += KNIGHT_OUTPOST_ON_HOLE;
        }

        // add bonus for knight mobility
        int mobility = popCount(KNIGHT_ATTACKS[square] & ~(enemyPawnAttacks | board.getOccupied(color)));
        score += (int)(KNIGHT_MOBILITY_MULTIPLIER * sqrt((double)mobility)) - KNIGHT_MOBILITY_OFFSET;
        //score += KNIGHT_MOBILITY_TABLE[mobility];
        attackUnits += popCount(KNIGHT_ATTACKS[square] & safetyArea) * MINOR_ATTACK_UNITS;

        knights &= knights - 1;
    }

    return score;
}

// rook
bool openFile(Board& board, Color color, int file)
{
    Color enemy = static_cast<Color>(1 - color);
    UInt64 allyPawns = board.getPiece(color, PAWN), enemyPawns = board.getPiece(enemy, PAWN);

    // return true if there are no pawns on the file
    return !((allyPawns | enemyPawns) & FILE_MASKS[file]);
}

bool halfOpenFile(Board& board, Color color, int file)
{
    Color enemy = static_cast<Color>(1 - color);
    UInt64 allyPawns = board.getPiece(color, PAWN), enemyPawns = board.getPiece(enemy, PAWN);

    // return true if we don't have a pawn but enemy does
    return !(allyPawns & FILE_MASKS[file]) && (enemyPawns & FILE_MASKS[file]);
}

bool kingBlockRook(Board& board, Color color, Square rookSquare)
{
    // get king square
    Square kingSquare = static_cast<Square>(lsb(board.getPiece(color, KING)));

    // check for patterns
    if (color == WHITE)
    {
        if (rookSquare == h1)
        {
            if (kingSquare == g1 || kingSquare == f1)
            {
                return true;
            }
        }
        else if (rookSquare == a1)
        {
            if (kingSquare == b1 || kingSquare == c1 || kingSquare == d1)
            {
                return true;
            }
        }
    }
    else
    {
        if (rookSquare == h8)
        {
            if (kingSquare == g8 || kingSquare == f8)
            {
                return true;
            }
        }
        else if (rookSquare == a8)
        {
            if (kingSquare == b8 || kingSquare == c8 || kingSquare == d8)
            {
                return true;
            }
        }
    }

    return false;
}

void rookScore(Board& board, Color color, UInt64 enemyPawnAttacks, int& openingScore, int& endgameScore, int& attackUnits, UInt64 safetyArea)
{
    // loop through all rooks
    UInt64 rooks = board.getPiece(color, ROOK);
    Square square;
    UInt64 rookAttack;
    while (rooks)
    {
        square = static_cast<Square>(lsb(rooks));

        // check if rook is on open file
        int file = square % 8;
        if (openFile(board, color, file))
        {
            openingScore += ROOK_OPEN_FILE;
        }

        // check if rook is blocked by king
        if (kingBlockRook(board, color, square))
        {
            openingScore -= KING_BLOCK_ROOK_PENALTY;
        }

        // rook attack
        rookAttack = lookupRookAttack(square, board.getFullOccupied() ^ (1ULL << square));
        int horizontalMobility = popCount(rookAttack & RANK_MASKS[square / 8] & ~(enemyPawnAttacks | board.getOccupied(color)));
        int verticalMobility = popCount(rookAttack & FILE_MASKS[square % 8] & ~(enemyPawnAttacks | board.getOccupied(color)));
        openingScore += (int)(ROOK_HORIZONTAL_MOBILITY_MULTIPLIER * sqrt((double)horizontalMobility)) - ROOK_HORIZONTAL_MOBILITY_OFFSET;
        endgameScore += (int)(ROOK_VERTICAL_MOBILITY_MULTIPLIER * sqrt((double)verticalMobility)) - ROOK_VERTICAL_MOBILITY_OFFSET;
        //openingScore += ROOK_HORIZONTAL_MOBILITY_TABLE[horizontalMobility];
        //endgameScore += ROOK_VERTICAL_MOBILITY_TABLE[verticalMobility];
        attackUnits += popCount(rookAttack & safetyArea) * ROOK_ATTACK_UNITS;
        if (rookAttack & board.getPiece(static_cast<Color>(1 - color), KING)) attackUnits += ROOK_CHECK_UNITS;

        rooks &= rooks - 1;
    }

    // connect rooks
    UInt64 rookIntersect = rookAttack & board.getPiece(color, ROOK);
    if (rookIntersect)
    {
        openingScore += ROOK_CONNECTED;
        endgameScore += ROOK_CONNECTED;
    }
}

// pawn
bool isPassed(Board& board, Color color, Square square)
{
    int rank = square / 8, file = square % 8;
    UInt64 neighboringFiles = NEIGHBORING_FILES[file];
    UInt64 frontSpan = color == WHITE ? SQUARES_ABOVE_WHITE_PAWNS[rank] : SQUARES_BELOW_BLACK_PAWNS[rank];

    // if pawn is obstructed by ally pawn, return false
    if (frontSpan & FILE_MASKS[file] & board.getPiece(color, PAWN))
    {
        return false;
    }

    return !(frontSpan & neighboringFiles & board.getPiece(static_cast<Color>(1 - color), PAWN));
}

bool isUnstoppable(Board& board, Color color, Square square)
{
    // assumes we're dealing with a passed pawn that is unobstructed
    if (board.getOccupied(static_cast<Color>(1-color)) == (board.getPiece(static_cast<Color>(1-color), KING) | board.getPiece(static_cast<Color>(1-color), PAWN)))
    {
        // if enemy only has king and pawn, use rule of square
        int pawnRank = square / 8;
        int promoRank = color == WHITE ? 7 : 0, promoFile = square % 8;
        int enemyKingSquare = lsb(board.getPiece(static_cast<Color>(1-color), KING));
        int enemyKingRank = enemyKingSquare / 8, enemyKingFile = enemyKingSquare % 8;

        // use chebyshev distance to determine if pawn is unstoppable
        int pawnDistance = abs(pawnRank - promoRank);
        int kingDistance = std::max(abs(enemyKingRank - promoRank), abs(enemyKingFile - promoFile));
        if (std::min(5, pawnDistance) < kingDistance - (board.getNextMove() == 1 - color))
        {
            return true;
        }
    }

    return false;
}
bool isCandidate(Board& board, Color color, Square square)
{
    // get frontspan
    int rank = square / 8, file = square % 8;
    UInt64 frontSpan = color == WHITE ? SQUARES_ABOVE_WHITE_PAWNS[rank] : SQUARES_BELOW_BLACK_PAWNS[rank];
    frontSpan &= FILE_MASKS[file];

    // if pawn is obstructed by any pawn, return false
    if ( frontSpan & (board.getPiece(color, PAWN) | board.getPiece(static_cast<Color>(1 - color), PAWN)) )
    {
        return false;
    }

    // iterate through each square in the frontspan: if no square is attacked by more enemy pawns than ally pawns, this pawn is a candidate
    while (frontSpan)
    {
        Square frontSquare = static_cast<Square>(lsb(frontSpan));
        UInt64 enemyPawnAttacks = PAWN_ATTACKS[color][frontSquare] & board.getPiece(static_cast<Color>(1 - color), PAWN);
        UInt64 allyPawnAttacks = PAWN_ATTACKS[1 - color][frontSquare] & board.getPiece(color, PAWN);
        if (popCount(enemyPawnAttacks) > popCount(allyPawnAttacks))
        {
            return false;
        }

        frontSpan &= frontSpan - 1;
    }

    return true;
}

bool isObstructed(Board& board, Color color, Square square)
{
    // get frontspan
    int rank = square / 8, file = square % 8;
    UInt64 frontSpan = color == WHITE ? SQUARES_ABOVE_WHITE_PAWNS[rank] : SQUARES_BELOW_BLACK_PAWNS[rank];
    frontSpan &= FILE_MASKS[file];

    // if pawn is obstructed by any piece, return true
    if ( frontSpan & board.getFullOccupied() )
    {
        return true;
    }

    return false;
}

bool isProtected(Board& board, Color color, Square square)
{
    return PAWN_ATTACKS[1 - color][square] & board.getPiece(color, PAWN);
} 

bool isIsolated(Board& board, Color color, Square square)
{
    int file = square % 8;
    UInt64 neighboringFiles = NEIGHBORING_FILES[file] ^ FILE_MASKS[file];
    return !(neighboringFiles & board.getPiece(color, PAWN));
}

bool isBackward(Board& board, Color color, Square square)
{
    // get the backspan of the pawn
    int rank = square / 8;
    UInt64 backSpan = color == WHITE ? SQUARES_BELOW_BLACK_PAWNS[rank] : SQUARES_ABOVE_WHITE_PAWNS[rank];
    UInt64 neighboringFiles = NEIGHBORING_FILES[square % 8] ^ FILE_MASKS[square % 8];
    if (backSpan & neighboringFiles & board.getPiece(color, PAWN))
    {
        return false;
    }

    // get the stop square of the pawn
    Square stopSquare = color == WHITE ? static_cast<Square>(square + 8) : static_cast<Square>(square - 8);

    // if stop square is protected by ally pawn, then it is not backward
    if (isProtected(board, color, stopSquare))
    {
        return false;
    }

    // if stop square is not attacked by enemy pawn, then it is not backward
    if (!isProtected(board, static_cast<Color>(1 - color), stopSquare))
    {
        return false;
    }

    return true;
}

void pawnScore(Board& board, Color color, UInt64& attacks, int& openingScore, int& endgameScore)
{
    // loop through all pawns
    UInt64 pawns = board.getPiece(color, PAWN);
    while (pawns)
    {
        Square square = static_cast<Square>(lsb(pawns));

        // add attacks to attacks bitboard
        attacks |= PAWN_ATTACKS[color][square];

        // check if pawn is passed
        if (isPassed(board, color, square))
        {
            // unobstructed
            bool obstructed = isObstructed(board, color, square);
            if (!obstructed)
            {
                // if pawn is unstoppable, add bonus
                if (isUnstoppable(board, color, square))
                {
                    endgameScore += UNSTOPPABLE_PASSED_PAWN;
                }
                else
                {
                    openingScore += UNOBSTRUCTED_PASSER;
                    endgameScore += UNOBSTRUCTED_PASSER;
                }
            }
            else 
            {
                openingScore += PASSED_PAWN;
                endgameScore += PASSED_PAWN;
            }
        }

        // check if pawn is candidate
        else if (isCandidate(board, color, square))
        {
            // unobstructed bonus
            bool obstructed = isObstructed(board, color, square);
            if (!obstructed)
            {
                openingScore += UNOBSTRUCTED_CANDIDATE;
                endgameScore += UNOBSTRUCTED_CANDIDATE;
            }
            else 
            {
                openingScore += CANDIDATE_PASSED_PAWN;
                endgameScore += CANDIDATE_PASSED_PAWN;
            }
        }

        // check if pawn is isolated
        if (isIsolated(board, color, square))
        {
            openingScore -= ISOLATED_PAWN_PENALTY;
        }

        // check if pawn is backward
        else if (isBackward(board, color, square))
        {
            openingScore -= BACKWARD_PAWN_PENALTY;
        }
        
        pawns &= pawns - 1;
    }
}

// king
int kingPawnShieldScore(Board& board, UInt64 safetyArea, Color color)
{
    return popCount(safetyArea & board.getPiece(color, PAWN));
}

int kingPawnStormScore(Board& board, UInt64 dangerArea, Color color)
{
    return popCount(dangerArea & board.getPiece(static_cast<Color>(1 - color), PAWN));
}

UInt64 kingSafetyArea(Color color, Square square)
{
    UInt64 safetyArea = KING_ATTACKS[square];
    int rank = square / 8;

    // add rank 2 ranks ahead of king
    if (color == WHITE)
    {
        if (rank < 6) safetyArea |= (RANK_MASKS[rank + 2] & NEIGHBORING_FILES[square % 8]);
        if (rank > 0) safetyArea &= ~RANK_MASKS[rank - 1];
    }
    else
    {
        if (rank > 1) safetyArea |= (RANK_MASKS[rank - 2] & NEIGHBORING_FILES[square % 8]);
        if (rank < 7) safetyArea &= ~RANK_MASKS[rank + 1];
    }

    return safetyArea;
}

// king safety area + one extra rank in front of king
UInt64 kingDangerArea(Color color, Square square)
{
    UInt64 dangerArea = kingSafetyArea(color, square);
    int rank = square / 8;

    // add rank 3 ranks ahead of king
    if (color == WHITE)
    {
        if (rank < 5) dangerArea |= (RANK_MASKS[rank + 3] & NEIGHBORING_FILES[square % 8]);
    }
    else
    {
        if (rank > 2) dangerArea |= (RANK_MASKS[rank - 3] & NEIGHBORING_FILES[square % 8]);
    }

    return dangerArea;
}