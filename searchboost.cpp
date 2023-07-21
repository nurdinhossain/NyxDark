#include "searchboost.h"
#include "evaluate.h"
#include "bitboard.h"
#include "tables.h"
#include "evalparams.h"
#include <vector>
#include <algorithm>
#include <iostream>

// methods for boosting search
bool moveCausesCheck(Board& board, Move move)
{
    // NOTE: we don't worry about rare "promotion checks" here because promo moves are never pruned in the first place

    Piece piece = extractPiece(board.getSquareToPiece(move.from));
    Color enemy = static_cast<Color>(1 - board.getNextMove());
    UInt64 enemyKing = board.getPiece(enemy, KING);
    Square enemyKingSquare = static_cast<Square>(lsb(enemyKing));

    // if the attacker is a knight, return true if its move attacks the enemy king
    if (piece == KNIGHT)
    {
        if (KNIGHT_ATTACKS[move.to] & enemyKing)
        {
            return true;
        }
    }

    // if the attacker is a pawn, return true if its move attacks the enemy king
    if (piece == PAWN)
    {
        if (PAWN_ATTACKS[board.getNextMove()][move.to] & enemyKing)
        {
            return true;
        }
    }

    // create occupancy board
    UInt64 occupancy = board.getFullOccupied() ^ (1ULL << move.from) ^ (1ULL << move.to) ^ (1ULL << enemyKingSquare);

    // generate bishop and rook attacks from enemy king square
    UInt64 bishopAttackFromKingSquare = lookupBishopAttack(enemyKingSquare, occupancy);
    UInt64 rookAttackFromKingSquare = lookupRookAttack(enemyKingSquare, occupancy);

    // get intersection of bishop and rook attacks from enemy king square with ally pieces
    UInt64 bishopAttackIntersection = bishopAttackFromKingSquare & (board.getPiece(board.getNextMove(), BISHOP) | board.getPiece(board.getNextMove(), QUEEN));
    UInt64 rookAttackIntersection = rookAttackFromKingSquare & (board.getPiece(board.getNextMove(), ROOK) | board.getPiece(board.getNextMove(), QUEEN));
    
    // if the general intersection is not empty, return true
    if (bishopAttackIntersection || rookAttackIntersection)
    {
        return true;
    }

    // if path intersects with the piece moved and it is queen/bishop/rook, return true
    if (bishopAttackFromKingSquare & (1ULL << move.to) && (piece == BISHOP || piece == QUEEN))
    {
        return true;
    }

    if (rookAttackFromKingSquare & (1ULL << move.to) && (piece == ROOK || piece == QUEEN))
    {
        return true;
    }

    return false;
}

bool lmpOk(Board& board, Move move, int moveIndex, int depth)
{
    // if depth is greater than LMP_MAX_DEPTH, return false
    if (depth > LMP_MAX_DEPTH)
    {
        return false;
    }

    // if move is not quiet, return false
    if (move.type != QUIET)
    {
        return false;
    }

    // if moveIndex is below lmp limit for this depth, return false
    if (moveIndex < LMP_MAX_MOVES[depth])
    {
        return false;
    }

    return true;
}

bool lmrValid(Board& board, Move move, int moveIndex, int depth)
{
    // if moveIndex is greater than LMR_LIMIT, return false
    if (moveIndex > LMR_LIMIT)
    {
        return false;
    }

    // if depth is less than 3, return false
    if (depth < LMR_MIN_DEPTH)
    {
        return false;
    }

    // if move is not quiet, return false
    if (move.type != QUIET)
    {
        return false;
    }

    return true;
}

int lmrReduction(int moveIndex, int depth)
{
    if (moveIndex < 6) return 1;

    return depth / 3;
}

bool futile(Board& board, Move move, int moveIndex, int depth, int alpha, int beta)
{
    // if depth is greater than FUTILE_MAX_DEPTH, return false
    if (depth > FUTILE_MAX_DEPTH)
    {
        return false;
    }

    // if first move, return false
    if (moveIndex == 0)
    {
        return false;
    }

    // if move is promotion, return false
    if (move.type >= KNIGHT_PROMOTION)
    {
        return false;
    }

    // if piece moved is a passed pawn, return false
    if (extractPiece(board.getSquareToPiece(move.from)) == PAWN)
    {
        if (isPassed(board, board.getNextMove(), move.from))
            return false;
    }

    // if alpha or beta is near checkmate, return false
    if (abs(alpha) > MATE - MATE_BUFFER || abs(beta) > MATE - MATE_BUFFER)
    {
        return false;
    }
    
    // margin
    int margin = FUTILE_MARGINS[depth];
    
    // capture gain
    if (move.pieceTaken != EMPTY)
    {
        margin += PIECE_VALUES[move.pieceTaken - 1];
    }

    // if move could raise alpha, return false. otherwise, return true
    if (lazyEvaluate(board) + margin <= alpha)
    {
        return true;
    }

    return false;
}

bool razorOk(Board& board, int depth, int alpha)
{
    // if depth is not equal to RAZOR_DEPTH, return false
    if (depth != RAZOR_DEPTH)
    {
        return false;
    }

    // if alpha is near checkmate, return false
    if (abs(alpha) > MATE - MATE_BUFFER)
    {
        return false;
    }

    int score = lazyEvaluate(board) + RAZOR_MARGIN;
    return score <= alpha && popCount(board.getOccupied(static_cast<Color>(1-board.getNextMove()))) > 3;
}

bool reverseFutileOk(Board& board, int depth,  int beta)
{
    // if depth is greater than REVERSE_FUTILE_MAX_DEPTH, return false
    if (depth > REVERSE_FUTILE_MAX_DEPTH)
    {
        return false;
    }

    // if beta is near checkmate, return false
    if (abs(beta) > MATE - MATE_BUFFER)
    {
        return false;
    }

    return true;
}

bool nullOk(Board& board, int depth)
{
    // if depth is less than NULL_DEPTH, return false
    if (depth < NULL_DEPTH)
    {
        return false;
    }

    return true;
}

int see(Board& board, Square initialFrom, Square initialTo)
{
    // get a bitboard of all the shallow (not considering x-ray) attackers of the "to" square
    UInt64 fullAttackers = board.getAttackersForSquare(initialTo);
    UInt64 shallowAttackers = fullAttackers;

    // pseudo-occupancy bitboard
    UInt64 occupancy = board.getFullOccupied();

    // loop through the attackers and sort them into a list by their value
    Square attackers[MAX_PIECES];
    Square defenders[MAX_PIECES];
    int attackersIndex = 0, defendersIndex = 0;
    while (shallowAttackers)
    {
        // get the least significant bit
        Square attacker = static_cast<Square>(lsb(shallowAttackers));

        // remove the least significant bit
        shallowAttackers ^= (1ULL << attacker);

        // if attacker is piece on initialFrom, skip it (we will add it later)
        if (attacker == initialFrom)
        {
            continue;
        }

        // add piece to appropriate list
        Color color = extractColor(board.getSquareToPiece(attacker));
        if (color == board.getNextMove())
        {
            attackers[attackersIndex++] = attacker;
        }
        else
        {
            defenders[defendersIndex++] = attacker;
        }
    }

    // sort the attackers and defenders by value (most valuable first)
    std::sort(attackers, attackers + attackersIndex, [&board](Square a, Square b) {
        return PIECE_VALUES[extractPiece(board.getSquareToPiece(a)) - 1] > PIECE_VALUES[extractPiece(board.getSquareToPiece(b)) - 1];
    });
    std::sort(defenders, defenders + defendersIndex, [&board](Square a, Square b) {
        return PIECE_VALUES[extractPiece(board.getSquareToPiece(a)) - 1] > PIECE_VALUES[extractPiece(board.getSquareToPiece(b)) - 1];
    });

    // add the piece on initialFrom to the attackers list
    attackers[attackersIndex++] = initialFrom;

    // loop
    int score = 0;
    int mode = 1; // 0 = enemy defending, 1 = ally attacking
    Piece pieceOnToSquare = extractPiece(board.getSquareToPiece(initialTo));
    while (true)
    {
        // if we're in ally attacking mode and there are no attackers, break
        if (mode == 1 && attackersIndex == 0)
        {
            break;
        }

        // if we're in enemy defending mode and there are no defenders, break
        if (mode == 0 && defendersIndex == 0)
        {
            break;
        }

        // get the next piece and pop it off the list end
        Square nextPieceSquare;
        if (mode == 1)
        {
            nextPieceSquare = attackers[attackersIndex - 1];
            attackersIndex--;
        }
        else
        {
            nextPieceSquare = defenders[defendersIndex - 1];
            defendersIndex--;
        }

        // get the piece on the square
        Piece nextPiece = extractPiece(board.getSquareToPiece(nextPieceSquare));

        // simulate the capture
        Piece capturedPiece = pieceOnToSquare;
        pieceOnToSquare = nextPiece;
        occupancy ^= (1ULL << nextPieceSquare);

        // get the new score
        if (mode == 1)
        {
            score += PIECE_VALUES[capturedPiece - 1];
        }
        else
        {
            score -= PIECE_VALUES[capturedPiece - 1];
        }

        // if it is the ally's turn and the score is negative, return score because defender can stand pat
        if (mode == 1 && score < 0)
        {
            return score;
        }

        // if it is the enemy's turn and the score is positive, return score because attacker can stand pat
        if (mode == 0 && score >= 0)
        {
            return score;
        }

        // add any discovered attackers to the list (if the piece is a queen, rook, or bishop)
        if (nextPiece != KNIGHT) // knights cannot be pinned
        {
            int row = nextPieceSquare / 8, col = nextPieceSquare % 8;
            if ((FILE_MASKS[col] & (1ULL << initialTo)) || (RANK_MASKS[row] & (1ULL << initialTo))) // if the piece is on the same file or rank as the initial capture
            {
                shallowAttackers = lookupRookAttack(initialTo, occupancy ^ (1ULL << initialTo)) & (board.getPiece(static_cast<Color>(1-board.getNextMove()), ROOK) | board.getPiece(static_cast<Color>(1-board.getNextMove()), QUEEN) | board.getPiece(board.getNextMove(), ROOK) | board.getPiece(board.getNextMove(), QUEEN)) & ~fullAttackers;
            }
            else
            {
                shallowAttackers = lookupBishopAttack(initialTo, occupancy ^ (1ULL << initialTo)) & (board.getPiece(static_cast<Color>(1-board.getNextMove()), BISHOP) | board.getPiece(static_cast<Color>(1-board.getNextMove()), QUEEN) | board.getPiece(board.getNextMove(), BISHOP) | board.getPiece(board.getNextMove(), QUEEN)) & ~fullAttackers;
            }

            // add the discovered attackers to the list
            fullAttackers |= shallowAttackers;
            while (shallowAttackers)
            {
                // get the least significant bit
                Square attacker = static_cast<Square>(lsb(shallowAttackers));

                // remove the least significant bit
                shallowAttackers ^= (1ULL << attacker);

                // add piece to appropriate list
                Color color = extractColor(board.getSquareToPiece(attacker));
                if (color == board.getNextMove())
                {
                    attackers[attackersIndex++] = attacker;
                }
                else
                {
                    defenders[defendersIndex++] = attacker;
                }
            }

            // sort the attackers and defenders by value (most valuable first)
            std::sort(attackers, attackers + attackersIndex, [&board](Square a, Square b) {
                return PIECE_VALUES[extractPiece(board.getSquareToPiece(a)) - 1] > PIECE_VALUES[extractPiece(board.getSquareToPiece(b)) - 1];
            });
            std::sort(defenders, defenders + defendersIndex, [&board](Square a, Square b) {
                return PIECE_VALUES[extractPiece(board.getSquareToPiece(a)) - 1] > PIECE_VALUES[extractPiece(board.getSquareToPiece(b)) - 1];
            });
        }

        // switch modes
        mode = 1 - mode;
    }

    // return the score
    return score;
}