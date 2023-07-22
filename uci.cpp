#include "uci.h"
#include "game.h"
#include "search.h"
#include "transposition.h"
#include "book.h"
#include <iostream>
#include <sstream>

// method for calculating time
int getTime(int timeLeft, int inc, int moveNumber)
{
    return timeLeft / max(20, 45 - moveNumber) + inc;
}

// method for uci protocol
void uci(unordered_map<string, vector<string>> book)
{
    Board* board = new Board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    AI* master = new AI();
    TranspositionTable* tt = new TranspositionTable(TT_SIZE);
    bool DEBUG = false;
    int moveNumber = 0;

    string input;
    while (true)
    {
        fflush(stdout);
        getline(cin, input);

        if (input.substr(0, 10) == "ucinewgame")
        {
            // reset board
            if (board != NULL)
                delete board;

            board = new Board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

            // reset transposition table
            tt->clear();

            // reset AI
            if (master != NULL)
                delete master;
            master = new AI();

            // reset move number
            moveNumber = 0;
        }
        else if (input.substr(0, 8) == "position")
        {
            // position startpos moves e2e4 e7e5 b7b8q
            moveNumber = 0;
            string fen = "";
            int movesIndex = input.find("moves");
            if (input.substr(9, 8) == "startpos")
            {
                fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
            }
            else
            {
                // position fen rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w
                // KQkq - 0 1 moves e2e4 e7e5 b7b8q
                // if does not contain moves, then it is just a fen
                if (movesIndex == string::npos)
                {
                    fen = input.substr(13, input.length() - 13);
                }
                else
                {
                    // if it does contain moves, then we can go until the word moves
                    fen = input.substr(13, movesIndex - 14);
                }
            }

            // reset board
            if (board != NULL)
                delete board;
            board = new Board(fen);

            // parse moves
            Move legalMoves[MAX_MOVES];
            if (movesIndex != string::npos)
            {
                string moves = input.substr(movesIndex + 6, input.length() - movesIndex - 6);
                vector<string> moveList = split(moves, ' ');

                // loop through moves
                for (string move : moveList)
                {
                    // increment move number
                    moveNumber++;

                    // split 4 char move into 2 char from and to
                    string from = move.substr(0, 2);
                    string to = move.substr(2, 2);

                    // convert to index
                    int fromIndex = squareToIndex(from);
                    int toIndex = squareToIndex(to);

                    // isolate promotion piece if exists
                    Piece promotionPiece = EMPTY;
                    if (move.length() == 5)
                    {
                        char promotionChar = move[4];
                        switch (promotionChar)
                        {
                            case 'q':
                                promotionPiece = QUEEN;
                                break;
                            case 'r':
                                promotionPiece = ROOK;
                                break;
                            case 'b':
                                promotionPiece = BISHOP;
                                break;
                            case 'n':
                                promotionPiece = KNIGHT;
                                break;
                        }
                    }

                    // get list of legal moves
                    board->moveGenerationSetup();
                    int numMoves = 0;
                    board->generateMoves(legalMoves, numMoves, false);

                    // loop through legal moves
                    for (int i = 0; i < numMoves; i++)
                    {
                        // if move is legal, then make it
                        if (legalMoves[i].from == fromIndex && legalMoves[i].to == toIndex)
                        {
                            Piece correctPromoPiece = EMPTY;
                            if (legalMoves[i].type >= KNIGHT_PROMOTION_CAPTURE)
                                correctPromoPiece = static_cast<Piece>(legalMoves[i].type - KNIGHT_PROMOTION_CAPTURE + 2);
                            else if (legalMoves[i].type >= KNIGHT_PROMOTION)
                                correctPromoPiece = static_cast<Piece>(legalMoves[i].type - KNIGHT_PROMOTION + 2);

                            if (correctPromoPiece == promotionPiece)
                            {
                                board->makeMove(legalMoves[i]);
                                break;
                            }
                        }
                    }
                }
            }

            // reset AI only if NULL
            if (master == NULL)
                master = new AI();
        }
        else if (input.substr(0, 5) == "debug")
        {
            if (input.substr(6, 3) == "on")
                DEBUG = true;
            else
                DEBUG = false;
        }
        else if (input.substr(0, 7) == "isready")
        {
            std::cout << "readyok" << endl;
        }
        else if (input == "uci")
        {
            std::cout << "id name " << ENGINE_NAME << endl;
            std::cout << "id author " << ENGINE_AUTHOR << endl;
            std::cout << "uciok" << endl;
        }
        else if (input.substr(0, 4) == "quit")
        {
            break;
        }
        else if (input == "d")
        {
            if (board != NULL)
                board->print();
        }
        else if (input.substr(0, 2) == "go")
        {
            // look for book move
            if (book.find(board->getFen()) != book.end())
            {
                vector<string> moves = book[board->getFen()];
                int index = rand() % moves.size();
                string move = moves[index];
                string from = move.substr(0, 2);
                string to = move.substr(2, 2);
                std::cout << "bestmove " << from << to << endl;
                continue;
            }

            // parse arguments for wtime, btime, winc, binc, movestogo, movetime, and infinite
            int wtime = -1;
            int btime = -1;
            int winc = -1;
            int binc = -1;
            int movestogo = -1;
            int movetime = -1;
            bool infinite = false;
            vector<string> tokens = split(input, ' ');
            for (int i = 0; i < tokens.size(); i++)
            {
                if (tokens[i] == "wtime")
                    wtime = stoi(tokens[i + 1]);
                else if (tokens[i] == "btime")
                    btime = stoi(tokens[i + 1]);
                else if (tokens[i] == "winc")
                    winc = stoi(tokens[i + 1]);
                else if (tokens[i] == "binc")
                    binc = stoi(tokens[i + 1]);
                else if (tokens[i] == "movestogo")
                    movestogo = stoi(tokens[i + 1]);
                else if (tokens[i] == "movetime")
                    movetime = stoi(tokens[i + 1]);
                else if (tokens[i] == "infinite")
                    infinite = true;
            }

            // set max time
            if (board->getNextMove() == WHITE)
            {
                if (wtime != -1)
                    MAX_TIME = getTime(wtime, winc, moveNumber);
                else if (btime != -1)
                    MAX_TIME = getTime(btime, binc, moveNumber);
                else if (movetime != -1)
                    MAX_TIME = movetime;
                else if (infinite)
                    MAX_TIME = 1000000000;
            }
            else
            {
                if (btime != -1)
                    MAX_TIME = getTime(btime, binc, moveNumber);
                else if (wtime != -1)
                    MAX_TIME = getTime(wtime, winc, moveNumber);
                else if (movetime != -1)
                    MAX_TIME = movetime;
                else if (infinite)
                    MAX_TIME = 1000000000;
            }

            Move bestMove = master->getBestMove(*board, tt, true);

            // print best move in uci format
            std::string stringMove = indexToSquare(bestMove.from) + indexToSquare(bestMove.to);
            if (bestMove.type == KNIGHT_PROMOTION || bestMove.type == KNIGHT_PROMOTION_CAPTURE) stringMove += "n";
            else if (bestMove.type == BISHOP_PROMOTION || bestMove.type == BISHOP_PROMOTION_CAPTURE) stringMove += "b";
            else if (bestMove.type == ROOK_PROMOTION || bestMove.type == ROOK_PROMOTION_CAPTURE) stringMove += "r";
            else if (bestMove.type == QUEEN_PROMOTION || bestMove.type == QUEEN_PROMOTION_CAPTURE) stringMove += "q";
            std::cout << "bestmove " << stringMove << std::endl;
        }
    };

    delete board;
    delete master;
    delete tt;
}