#include <iostream>
#include <chrono>
#include <cmath>
#include "client.h"
#include "search.h"
#include "evaluate.h"
#include "moveorder.h"
#include "searchboost.h"
#include "evalparams.h"
#include <thread>

int MAX_TIME;

// implement struct member functions
void SearchStats::print()
{
    std::cout << "Nodes: " << nodes;
    std::cout << ", QNodes: " << qNodes;
    std::cout << ", Cutoffs: " << cutoffs;
    std::cout << ", QCutoffs: " << qCutoffs;
    std::cout << ", TT Hits: " << ttHits;
    std::cout << ", Killers Stored: " << killersStored;
    std::cout << ", ReSearches: " << reSearches;
    std::cout << ", LMR Reductions: " << lmrReductions;
    std::cout << ", LMP Pruned: " << lmpPruned;
    std::cout << ", Futile Reductions: " << futileReductions;
    std::cout << ", Futile Reductions Q: " << futileReductionsQ;
    std::cout << ", Delta Pruned: " << deltaPruned;
    std::cout << ", SEE Pruned: " << seePruned;
    std::cout << ", Null Reductions: " << nullReductions;
    std::cout << ", Reverse Futile Pruned: " << reverseFutilePruned;
    std::cout << ", Razor Pruned: " << razorPruned;
    std::cout << ", MultiCut Pruned: " << multiCutPruned;
    std::cout << ", Extensions: " << extensions;
    std::cout << ", Singular Extensions: " << singularExtensions; 
    std::cout << ", IID Hits: " << iidHits << std::endl;
}

void SearchStats::clear()
{
    nodes = 0;
    qNodes = 0;
    cutoffs = 0;
    qCutoffs = 0;
    ttHits = 0;
    killersStored = 0;
    reSearches = 0;
    lmrReductions = 0;
    lmpPruned = 0;
    futileReductions = 0;
    futileReductionsQ = 0;
    deltaPruned = 0;
    seePruned = 0;
    nullReductions = 0;
    reverseFutilePruned = 0;
    razorPruned = 0;
    multiCutPruned = 0;
    extensions = 0;
    singularExtensions = 0;
    iidHits = 0;
}

/* IMPLEMENT AI METHODS */

// constructor/destructor
AI::AI()
{
    // set the pawn table size
    pawnTable_ = new PawnTable(PAWN_HASH_SIZE);
}

AI::~AI()
{
    // delete the pawn table
    delete pawnTable_;
}

// search
int AI::search(Board& board, TranspositionTable* transpositionTable_, int depth, int ply, int alpha, int beta, std::chrono::steady_clock::time_point start, bool& abort)
{
    // check for time
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end - start;
    if (elapsed.count() >= MAX_TIME)
    {
        abort = true;
        return DRAW;
    }

    // mate distance pruning
    alpha = std::max(alpha, -MATE + ply);
    beta = std::min(beta, MATE - ply);
    if (alpha >= beta)
    {
        return alpha;
    }

    // check if this is a pv node
    bool pvNode = (beta - alpha) > 1;

    // increment nodes
    searchStats_.nodes++;

    // insufficient material
    if (board.insufficientMaterial(WHITE) && board.insufficientMaterial(BLACK))
    {
        return DRAW;
    }

    // check for threefold repetition
    if (isRepetition(board))
    {
        return DRAW;
    }

    // check for fifty move rule
    if (board.getHistoryIndex() >= 100)
    {
        return DRAW;
    }  

    // check for max depth
    if (depth <= 0)
    {
        return quiesce(board, alpha, beta);
    }

    // transposition table lookup
    Entry* ttEntry = transpositionTable_->probe(board.getCurrentHash());
    UInt64 ttKey = ttEntry->key;
    int ttScore = FAIL_SCORE, ttDepth = 0;
    Flag ttFlag = NO_FLAG;
    Move ttMove = {BISHOP_PROMOTION_CAPTURE};

    if (ttKey == board.getCurrentHash())
    {
        // get info from tt
        ttDepth = ttEntry->depth;
        ttMove = ttEntry->move;
        ttScore = ttEntry->score;
        ttScore = transpositionTable_->correctScoreRead(ttScore, ply);
        ttFlag = ttEntry->flag;

        if (ttDepth >= depth)
        {
            if (ttFlag == EXACT)
            {
                searchStats_.ttHits++;
                return ttScore;
            }
            else if (ttFlag == LOWER_BOUND)
            {
                alpha = std::max(alpha, ttScore);
            }
            else if (ttFlag == UPPER_BOUND)
            {
                beta = std::min(beta, ttScore);
            }

            // check for cutoff
            if (alpha >= beta)
            {
                searchStats_.ttHits++;
                return ttScore;
            }
        }
    }

    // generate moves
    board.moveGenerationSetup();
    Move moves[MAX_MOVES];
    int numMoves = 0;
    board.generateMoves(moves, numMoves);

    // check for mate/stalemate
    bool friendlyKingInCheck = board.getCheckers() != 0;
    if (numMoves == 0)
    {
        if (friendlyKingInCheck)
        {
            return -MATE + ply;
        }
        else
        {
            return DRAW;
        }
    }

    /******************* 
     *     EXTENSIONS 
     *******************/
    int extensions = 0;
    
    // one-reply 
    if (numMoves == 1) 
    {
        searchStats_.extensions++;
        extensions++;
    }

    /******************* 
     *     PRUNING 
     *******************/
    if (!friendlyKingInCheck && !pvNode && extensions == 0)
    {
        // reliable eval score
        int reliableEval = evaluate(board, pawnTable_);

        // razoring
        if (razorOk(board, depth, alpha))
        {
            searchStats_.razorPruned++;
            depth -= 1;
        }

        // reverse futility pruning
        if (reverseFutileOk(board, depth, beta))
        {
            if (reliableEval - REVERSE_FUTILE_MARGINS[depth] >= beta)
            {
                searchStats_.reverseFutilePruned++;
                return beta;
            }
        }

        // extended null move reductions
        if (nullOk(board, depth) && reliableEval >= beta)
        {
            // make null move
            Square ep = board.makeNullMove();

            // get reduction
            int R = depth > 6 ? MAX_R : MIN_R;

            // search
            int score = -search(board, transpositionTable_, depth - R - 1, ply + 1, -beta, -beta + 1, start, abort);

            // undo null move
            board.unmakeNullMove(ep);

            // check for cutoff
            if (score >= beta)
            {
                depth -= DR;
                searchStats_.nullReductions++;
                if (depth <= 0)
                {
                    return reliableEval;
                }
            }
        }
    }

    /******************************
     * INTERNAL ITERATIVE DEEPENING 
     ******************************/
    if (ttScore == FAIL_SCORE && depth > MIN_IID_DEPTH)
    {
        // search with reduced depth
        search(board, transpositionTable_, depth - IID_DR, ply, alpha, beta, start, abort);

        // check if the tt entry is now valid
        ttKey = ttEntry->key;
        if (ttKey == board.getCurrentHash())
        {
            ttDepth = ttEntry->depth;
            ttMove = ttEntry->move;
            ttFlag = ttEntry->flag;
            ttScore = ttEntry->score;
            ttScore = transpositionTable_->correctScoreRead(ttScore, ply);
            searchStats_.iidHits++;
        }
    }

    /******************* 
     *     SEARCH 
     *******************/
    // sort moves 
    scoreMoves(board, ttMove, killerMoves_, moves, historyTable_, historyMax_, numMoves, ply);
    sortMoves(moves, numMoves);

    // initialize transposition flag and best move
    Flag flag = UPPER_BOUND;
    Move bestMove = moves[0];

    // loop through moves
    for (int i = 0; i < numMoves; i++)
    {
        // continue if excluded move
        if (moves[i].from == excludedMove_.from && moves[i].to == excludedMove_.to && moves[i].type == excludedMove_.type)
        {
            continue;
        }

        // see if move causes check
        bool causesCheck = moveCausesCheck(board, moves[i]);
        bool pruningOk = !causesCheck && !friendlyKingInCheck && !pvNode && extensions == 0;

        // late move pruning
        if (lmpOk(board, moves[i], i, depth) && pruningOk)
        {
            searchStats_.lmpPruned++;
            continue;
        }

        // futile pruning
        if (futile(board, moves[i], i, depth, alpha, beta) && pruningOk)
        {
            searchStats_.futileReductions++;
            continue;
        }

        // singular extension search
        bool singularExtension = false;
        if (ply > 0 && depth >= (4 + 2 * pvNode) && (moves[i].from == ttMove.from && moves[i].to == ttMove.to && moves[i].type == ttMove.type) && excludedMove_.from == NONE)
        {
            if (ttScore != FAIL_SCORE)
            {
                if (ttFlag == LOWER_BOUND && ttDepth >= depth - 3)
                {
                    int singularDepth = (depth - 1) / 2; 
                    excludedMove_ = moves[i];
                    int score = search(board, transpositionTable_, singularDepth, ply + 1, ttScore - 1, ttScore, start, abort);
                    excludedMove_ = {QUIET, NONE, NONE};

                    // singular extension
                    if (score < ttScore)
                    {
                        extensions++;
                        singularExtension = true;
                        searchStats_.singularExtensions++;
                    }

                    // otherwise, multi-cut prune
                    else if (score >= beta)
                    {
                        searchStats_.multiCutPruned++;
                        return beta;
                    }
                }
            }
        }

        // make move
        board.makeMove(moves[i]);

        // search
        int score;
        if (i == 0)
        {
            score = -search(board, transpositionTable_, depth - 1 + extensions, ply + 1, -beta, -alpha, start, abort);
        }
        else
        {
            // LMR 
            int reduction = 0;
            if (lmrValid(board, moves[i], i, depth) && pruningOk) reduction = lmrReduction(i, depth);

            // get score
            score = -search(board, transpositionTable_, depth - 1 - reduction + extensions, ply + 1, -alpha - 1, -alpha, start, abort);
            searchStats_.lmrReductions++;

            // re-search if necessary
            if (score > alpha && reduction > 0)
            {
                score = -search(board, transpositionTable_, depth - 1 + extensions, ply + 1, -beta, -alpha, start, abort);
                searchStats_.lmrReductions--;
                searchStats_.reSearches++;
            }
            else if (score > alpha && score < beta)
            {
                score = -search(board, transpositionTable_, depth - 1 + extensions, ply + 1, -beta, -alpha, start, abort);
                searchStats_.reSearches++;
            }
        }

        // undo move
        board.unmakeMove(moves[i]);

        // ensure that the fail scores don't f with the normal scores
        if (abort)
        {
            return DRAW;
        }

        // check for cutoff
        if (score >= beta)
        {
            // store in transposition table
            transpositionTable_->store(board.getCurrentHash(), LOWER_BOUND, depth, ply, score, moves[i]);

            // store killer move/update history if quiet move
            if (moves[i].type <= QUEEN_CASTLE)
            {
                Move firstKiller = killerMoves_[ply][0];

                // ensure that the killer move is not the same as the current move
                if (firstKiller.from != moves[i].from || firstKiller.to != moves[i].to || firstKiller.type != moves[i].type)
                {
                    killerMoves_[ply][1] = firstKiller;
                    killerMoves_[ply][0] = moves[i];
                    searchStats_.killersStored++;
                }

                // update history
                historyTable_[board.getNextMove()][moves[i].from][moves[i].to] += depth * depth;
                if (historyTable_[board.getNextMove()][moves[i].from][moves[i].to] > historyMax_)
                {
                    historyMax_ = historyTable_[board.getNextMove()][moves[i].from][moves[i].to];
                }
            }

            searchStats_.cutoffs++;
            return beta;
        }

        // update alpha
        if (score > alpha)
        {
            alpha = score;
            flag = EXACT;
            bestMove = moves[i];
        }

        // if singular extension, decrement extensions
        if (singularExtension)
        {
            extensions--;
        }
    }

    // store in transposition table
    transpositionTable_->store(board.getCurrentHash(), flag, depth, ply, alpha, bestMove);

    // return alpha
    return alpha;
}

// quiescence search
int AI::quiesce(Board& board, int alpha, int beta)
{
    // increment qNodes
    searchStats_.qNodes++;

    // evaluate board
    int score = evaluate(board, pawnTable_);

    // check for cutoff
    if (score >= beta)
    {
        searchStats_.qCutoffs++;
        return beta;
    }

    // delta pruning
    int DELTA = PIECE_VALUES[QUEEN - 1];
    if (score + DELTA < alpha)
    {
        searchStats_.deltaPruned++;
        return alpha;
    }

    // update alpha
    if (score > alpha)
    {
        alpha = score;
    }

    // generate attacking moves
    board.moveGenerationSetup();
    Move moves[MAX_MOVES_ATTACK];
    int numMoves = 0;
    board.generateMoves(moves, numMoves, true);

    // sort moves
    scoreMoves(board, {BISHOP_PROMOTION_CAPTURE}, killerMoves_, moves, historyTable_, historyMax_, numMoves, -1);
    sortMoves(moves, numMoves);

    // loop through moves
    for (int i = 0; i < numMoves; i++)
    {
        if (moves[i].type < KNIGHT_PROMOTION && moves[i].type != EN_PASSANT) // dont prune promos or ep
        {
            // see pruning
            int seeScore = see(board, moves[i].from, moves[i].to);
            if (seeScore < 0)
            {
                searchStats_.seePruned++;
                continue;
            }

            // futility pruning
            if (score + seeScore + FUTILE_MARGIN_Q < alpha)
            {
                searchStats_.futileReductionsQ++;
                continue;
            }
        }

        // make move
        board.makeMove(moves[i]);

        // search
        score = -quiesce(board, -beta, -alpha);

        // undo move
        board.unmakeMove(moves[i]);

        // check for cutoff
        if (score >= beta)
        {
            searchStats_.qCutoffs++;
            return beta;
        }

        // update alpha
        if (score > alpha)
        {
            alpha = score;
        }
    }

    // return alpha
    return alpha;
}

// history table methods
void AI::ageHistory()
{
    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < 64; j++)
        {
            for (int k = 0; k < 64; k++)
            {
                historyTable_[i][j][k] /= 2;
            }
        }
    }

    historyMax_ = std::max(historyMax_ / 2, 1);
}

Move AI::getBestMove(Board& board, TranspositionTable* transpositionTable_, bool verbose)
{
    // initialize variables
    Move bestMove = {QUIET, NONE, NONE};
    int bestScore = 0;
    int depth = 1;
    int alpha = NEG_INF, beta = POS_INF;
    int windowAlpha = 0, windowBeta = 0;
    bool abort = false;

    // age history table
    ageHistory();

    // iterative deepening with time
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    while (depth <= MAX_DEPTH)
    {
        // search
        int eval = search(board, transpositionTable_, depth, 0, alpha, beta, start, abort);

        // check for time
        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        std::chrono::duration<double, std::milli> elapsed = end - start;
        if (elapsed.count() >= MAX_TIME)
        {
            break;
        }

        // check if aspiration window was broken
        if (eval <= alpha)
        {
            windowAlpha += 1;
            alpha = bestScore - ASPIRATION_WINDOW[windowAlpha];
            searchStats_.reSearches++;
            continue;
        }
        else if (eval >= beta)
        {
            windowBeta += 1;
            beta = bestScore + ASPIRATION_WINDOW[windowBeta];
            searchStats_.reSearches++;
            continue;
        }
        else
        {
            // reset window
            windowAlpha = 0;
            windowBeta = 0;

            // update best move and score if the call was not broken prematurely and reset alpha and beta
            Move bestMoveCurrentIteration_ = transpositionTable_->probe(board.getCurrentHash())->move;

            bestMove = bestMoveCurrentIteration_;
            bestScore = eval;
            alpha = bestScore - ASPIRATION_WINDOW[windowAlpha];
            beta = bestScore + ASPIRATION_WINDOW[windowBeta];
        }

        if (verbose)
        {
            // print info
            /*std::cout << "depth: " << depth;
            std::cout << ", time: " << elapsed.count();
            std::cout << ", best move: " << indexToSquare(bestMove.from) << indexToSquare(bestMove.to);
            std::cout << ", score: " << bestScore / 100.0 << ", ";
            searchStats_.print();*/

            // print uci info
            std::cout << "info depth " << depth;
            std::cout << " time " << elapsed.count();
            std::cout << " nodes " << searchStats_.nodes;
            std::cout << " pv " << indexToSquare(bestMove.from) << indexToSquare(bestMove.to);
            std::cout << " score cp " << bestScore;
            std::cout << " nps " << (int)(searchStats_.nodes / elapsed.count()) * 1000;
            std::cout << std::endl;
        }

        // check for mate
        if (bestScore >= MATE - MAX_DEPTH || bestScore <= -MATE + MAX_DEPTH)
        {
            break;
        }

        // increment depth
        depth++;
    }

    searchStats_.clear();

    // return best move
    return bestMove;
}

// method for determining repetition
bool isRepetition(Board& board)
{
    int count = 0;

    for (int i = 0; i < board.getHistoryIndex(); i++)
    {
        if (board.getHistoryHash(i) == board.getCurrentHash())
        {
            count++;
        }
    }

    return count > 2;
}

