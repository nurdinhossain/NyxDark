#include "moveorder.h"
#include "tables.h"
#include "evaluate.h"
#include <iostream>

// score moves based on MVV/LVA
void scoreMoves(Board& board, Move ttMove, Move killerMoves[][2], Move moves[], int historyTable[2][64][64], int historyMax, int numMoves, int ply) 
{
    for (int i = 0; i < numMoves; i++) 
    {
        Move move = moves[i];
        moves[i].score = 0;

        // check for tt move
        if (move.from == ttMove.from && move.to == ttMove.to && move.type == ttMove.type)
        {
            moves[i].score = TT_MOVE;
            continue;
        }

        // check for capture
        if (move.pieceTaken != EMPTY)
        {
            // get piece moved
            int piece = board.getSquareToPiece(move.from);
            Piece pieceMoved = extractPiece(piece);

            // score move 
            moves[i].score = MVV_LVA[move.pieceTaken-1][pieceMoved-1] + CAPTURE_OFFSET;

            // check for promotion capture
            if (move.type >= KNIGHT_PROMOTION_CAPTURE && move.type <= QUEEN_PROMOTION_CAPTURE)
            {
                // score move based on promotion piece
                moves[i].score += PIECE_VALUES[move.type - 9] + PROMO_OFFSET;
            }

            continue;
        }

        // check for promotion
        if (move.type >= KNIGHT_PROMOTION && move.type <= QUEEN_PROMOTION)
        {
            // score move based on promotion piece
            moves[i].score = PIECE_VALUES[move.type - 5] + PROMO_OFFSET;
            continue;
        }

        // check for castle
        if (move.type == KING_CASTLE || move.type == QUEEN_CASTLE)
        {
            moves[i].score = CAPTURE_OFFSET;
            continue;
        }

        // check for pawn near promotion
        if (board.getSquareToPiece(move.from) == PAWN)
        {
            Color color = board.getNextMove();
            int rank = move.from / 8;
            if (color == WHITE && rank > 4 || color == BLACK && rank < 3)
            {
                moves[i].score = PAWN_NEAR_PROMO_OFFSET;

                // reward passed pawns
                if (isPassed(board, color, move.from)) 
                {
                    moves[i].score += 1;
                    
                    // reward unstoppable pawns
                    if (!isObstructed(board, color, move.from))
                    {
                        moves[i].score += 1;
                        if (isUnstoppable(board, color, move.from))
                            moves[i].score += 1; 
                    }
                }

                continue;
            }
        }

        // check for killer move
        if (ply >= 0)
            {
            if (move.from == killerMoves[ply][0].from && move.to == killerMoves[ply][0].to && move.type == killerMoves[ply][0].type)
            {
                moves[i].score = KILLER_OFFSET;
                continue;
            }
            else if (move.from == killerMoves[ply][1].from && move.to == killerMoves[ply][1].to && move.type == killerMoves[ply][1].type)
            {
                moves[i].score = KILLER_OFFSET - 1;
                continue;
            }
        }

        // check for history move
        moves[i].score = (int)((historyTable[board.getNextMove()][move.from][move.to] / (double)historyMax) * HISTORY_MULTIPLIER);
    }
}

// sort moves from highest score to lowest score using quicksort
void sortMoves(Move moves[], int numMoves) 
{
    // check for no moves
    if (numMoves == 0)
    {
        return;
    }

    // quicksort
    int i = 0;
    int j = numMoves - 1;
    Move pivot = moves[numMoves / 2];
    while (i <= j) 
    {
        while (moves[i].score > pivot.score)
        {
            i++;
        }
        while (moves[j].score < pivot.score)
        {
            j--;
        }
        if (i <= j) 
        {
            Move temp = moves[i];
            moves[i] = moves[j];
            moves[j] = temp;
            i++;
            j--;
        }
    }
    if (j > 0)
    {
        sortMoves(moves, j + 1);
    }
    if (i < numMoves)
    {
        sortMoves(moves + i, numMoves - i);
    }
}

// randomize move order
void randomizeMoves(Move moves[], int numMoves)
{
    // check for no moves
    if (numMoves == 0)
    {
        return;
    }

    // randomize move order
    for (int i = 0; i < numMoves; i++)
    {
        int j = rand() % numMoves;
        Move temp = moves[i];
        moves[i] = moves[j];
        moves[j] = temp;
    }
}