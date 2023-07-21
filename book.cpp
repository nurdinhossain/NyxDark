#include "book.h"
#include <fstream>
#include <iostream>
#include <algorithm>
using namespace std;

// method to read and preprocess a PGN file into separate games (represented by a vector of strings)
std::vector<std::vector<std::string>> processPGN(std::string filename, int limit)
{
    // initialize vector of games
    std::vector<std::vector<std::string>> games;
    std::vector<std::string> currentGame;
    int gameCount = 0;

    // open the file
    ifstream file(filename);

    // read the file
    string line;
    while (getline(file, line))
    {
        // remove all carriage returns
        line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());

        // if line contains a square bracket, forget it
        if (line.find('[') != string::npos)
        {
            continue;
        }
        
        // if line is empty/just contains newline, add copy of game to games and reset current game
        if (line == "" || line == "\n")
        {
            if (currentGame.size() > 0)
            {
                games.push_back(currentGame);
                currentGame.clear();
                gameCount++;

                // if game count is greater than limit, return games
                if (gameCount >= limit)
                {
                    return games;
                }
                
                // print number of games every 1000 games
                if (gameCount % 1000 == 0)
                {
                    cout << "Number of games: " << gameCount << endl;
                }
            }
        }

        // if line contains moves, split it by space
        else
        {
            // split line by space
            string delimiter = " ";
            size_t pos = 0;
            string token;
            while ((pos = line.find(delimiter)) != string::npos)
            {
                // get token
                token = line.substr(0, pos);

                // if token only has number and period, forget it
                if (token.find('.') != string::npos)
                {
                    line.erase(0, pos + delimiter.length());
                    continue;
                }

                // add token to current game
                currentGame.push_back(token);

                // remove token from line
                line.erase(0, pos + delimiter.length());
            }

            // add last token to current game (should be result)
            if (line.find('.') == string::npos)
                currentGame.push_back(line);
        }

        // if line is last line, add copy of game to games and reset current game
        if (file.eof())
        {
            if (currentGame.size() > 0)
            {
                games.push_back(currentGame);
                currentGame.clear();
            }
        }
    }

    // close the file
    file.close();

    // return games
    return games;
}

// method to search through a list of moves given a piece and seeing if a certain piece type can move to a certain square with different pieces
int ambiguousMove(Board& board, Move moves[], int moveCount, Piece piece, int square)
{
    // initialize variables
    int count = 0;
    int firstIndex = -1;

    // loop through moves
    for (int i = 0; i < moveCount; i++)
    {
        Piece fromPiece = extractPiece(board.getSquareToPiece(moves[i].from));
        // if move is to the same square and is a piece of the same type, increment count
        if (moves[i].to == square && fromPiece == piece)
        {
            count++;
            if (count > 1)
            {
                // 0 = no ambiguity, 1 = add file, 2 = add rank
                bool sameFile = firstIndex % 8 == moves[i].from % 8;
                if (sameFile) return 2;
                else return 1;
            }
            firstIndex = moves[i].from;
        }
    }

    return 0;
}

// method to convert game result ("1-0", "0-1", "1/2-1/2") into a number (1, 0, 0.5)
std::string processResult(std::string result)
{
    if (result == "1-0") return "1";
    else if (result == "0-1") return "0";
    else if (result == "1/2-1/2") return "0.5";
    
    // if we get here, throw an error
    throw std::invalid_argument("Invalid result");

    return "";
}

// method to convert a vector of strings (game) into fen strings
std::vector<std::string> processGame(std::vector<std::string> game, std::string startFen)
{
    // initialize board
    Board board = Board(startFen);

    // initialize vector of fen strings
    std::vector<std::string> fenStrings;

    // get result of game from last element of game vector
    std::string result = game[game.size() - 1];
    game.pop_back();

    // add initial fen
    fenStrings.push_back(board.getFen() + "," + processResult(result));

    // some lookup tables
    char pieceLookup[6] = { 'P', 'N', 'B', 'R', 'Q', 'K' };
    char fileLookup[8] = { 'h', 'g', 'f', 'e', 'd', 'c', 'b', 'a' };
    char rankLookup[8] = { '1', '2', '3', '4', '5', '6', '7', '8' };

    // loop through moves
    Move realMoves[256];
    for (int i = 0; i < game.size(); i++)
    {
        // move generation
        board.moveGenerationSetup();
        int moveCount = 0;
        board.generateMoves(realMoves, moveCount);

        std::string algebraicMove = game[i];

        // remove unnecessary characters like +, #, !, ?, etc.
        algebraicMove.erase(std::remove(algebraicMove.begin(), algebraicMove.end(), '+'), algebraicMove.end());
        algebraicMove.erase(std::remove(algebraicMove.begin(), algebraicMove.end(), '#'), algebraicMove.end());
        algebraicMove.erase(std::remove(algebraicMove.begin(), algebraicMove.end(), '!'), algebraicMove.end());
        algebraicMove.erase(std::remove(algebraicMove.begin(), algebraicMove.end(), '?'), algebraicMove.end());

        // loop through generated moves
        bool found = false;
        for (int j = 0; j < moveCount; j++)
        {
            // get move
            Move move = realMoves[j];
            Square from = move.from;
            Square to = move.to;
            Piece pieceMoved = extractPiece(board.getSquareToPiece(from));

            // try to recreate the algebraic move
            std::string recreatedMove = "";

            // pawn move
            if (pieceMoved == PAWN)
            {
                // if move is a capture, add file + x + square to
                if (move.pieceTaken != EMPTY)
                {
                    recreatedMove += fileLookup[from % 8];
                    recreatedMove += 'x';
                }
                recreatedMove += indexToSquare(to);

                // if move is a promotion, add = + piece
                if (move.type >= KNIGHT_PROMOTION)
                {
                    recreatedMove += '=';

                    if (move.type <= QUEEN_PROMOTION)
                        recreatedMove += pieceLookup[move.type - KNIGHT_PROMOTION + 1];
                    else
                        recreatedMove += pieceLookup[move.type - KNIGHT_PROMOTION_CAPTURE + 1];
                }
            }

            // castle
            else if (move.type == KING_CASTLE) recreatedMove += "O-O";
            else if (move.type == QUEEN_CASTLE) recreatedMove += "O-O-O";
            
            // non-pawn move
            else
            {
                // add piece
                recreatedMove += pieceLookup[pieceMoved - 1];

                // if move is ambiguous, add file or rank
                int ambiguous = ambiguousMove(board, realMoves, moveCount, pieceMoved, to);
                if (ambiguous == 1)
                {
                    recreatedMove += fileLookup[from % 8];
                }
                else if (ambiguous == 2)
                {
                    recreatedMove += rankLookup[from / 8];
                }

                // if move is a capture, add x + square to
                if (move.pieceTaken != EMPTY)
                {
                    recreatedMove += 'x';
                }
                recreatedMove += indexToSquare(to);
            }

            // compare recreated move to algebraic move
            if (recreatedMove == algebraicMove)
            {
                // make move
                board.makeMove(move);

                // add fen string to vector
                fenStrings.push_back(board.getFen() + "," + processResult(result));

                // set found to true
                found = true;
                break;
            }
        }

        // if move was not found, throw error
        if (!found)
        {
            std::cout << "Move not found: " << algebraicMove << std::endl;
            throw std::exception();
        }
    }

    // return fen strings
    return fenStrings;
}

// method just to process PGN moves
std::vector<std::string> processPGN(std::vector<std::string> game, std::string startFen)
{
    // initialize board
    Board board = Board(startFen);

    // initialize vector of fen strings
    std::vector<std::string> fenStrings;

    // add initial fen
    fenStrings.push_back(startFen);

    // some lookup tables
    char pieceLookup[6] = { 'P', 'N', 'B', 'R', 'Q', 'K' };
    char fileLookup[8] = { 'h', 'g', 'f', 'e', 'd', 'c', 'b', 'a' };
    char rankLookup[8] = { '1', '2', '3', '4', '5', '6', '7', '8' };

    // loop through moves
    Move realMoves[256];
    for (int i = 0; i < game.size(); i++)
    {
        // move generation
        board.moveGenerationSetup();
        int moveCount = 0;
        board.generateMoves(realMoves, moveCount);

        std::string algebraicMove = game[i];

        // remove unnecessary characters like +, #, !, ?, etc.
        algebraicMove.erase(std::remove(algebraicMove.begin(), algebraicMove.end(), '+'), algebraicMove.end());
        algebraicMove.erase(std::remove(algebraicMove.begin(), algebraicMove.end(), '#'), algebraicMove.end());
        algebraicMove.erase(std::remove(algebraicMove.begin(), algebraicMove.end(), '!'), algebraicMove.end());
        algebraicMove.erase(std::remove(algebraicMove.begin(), algebraicMove.end(), '?'), algebraicMove.end());

        // loop through generated moves
        bool found = false;
        for (int j = 0; j < moveCount; j++)
        {
            // get move
            Move move = realMoves[j];
            Square from = move.from;
            Square to = move.to;
            Piece pieceMoved = extractPiece(board.getSquareToPiece(from));

            // try to recreate the algebraic move
            std::string recreatedMove = "";

            // pawn move
            if (pieceMoved == PAWN)
            {
                // if move is a capture, add file + x + square to
                if (move.pieceTaken != EMPTY)
                {
                    recreatedMove += fileLookup[from % 8];
                    recreatedMove += 'x';
                }
                recreatedMove += indexToSquare(to);

                // if move is a promotion, add = + piece
                if (move.type >= KNIGHT_PROMOTION)
                {
                    recreatedMove += '=';

                    if (move.type <= QUEEN_PROMOTION)
                        recreatedMove += pieceLookup[move.type - KNIGHT_PROMOTION + 1];
                    else
                        recreatedMove += pieceLookup[move.type - KNIGHT_PROMOTION_CAPTURE + 1];
                }
            }

            // castle
            else if (move.type == KING_CASTLE) recreatedMove += "O-O";
            else if (move.type == QUEEN_CASTLE) recreatedMove += "O-O-O";
            
            // non-pawn move
            else
            {
                // add piece
                recreatedMove += pieceLookup[pieceMoved - 1];

                // if move is ambiguous, add file or rank
                int ambiguous = ambiguousMove(board, realMoves, moveCount, pieceMoved, to);
                if (ambiguous == 1)
                {
                    recreatedMove += fileLookup[from % 8];
                }
                else if (ambiguous == 2)
                {
                    recreatedMove += rankLookup[from / 8];
                }

                // if move is a capture, add x + square to
                if (move.pieceTaken != EMPTY)
                {
                    recreatedMove += 'x';
                }
                recreatedMove += indexToSquare(to);
            }

            // compare recreated move to algebraic move
            if (recreatedMove == algebraicMove)
            {
                // make move
                board.makeMove(move);

                // add fen string to vector
                fenStrings.push_back(board.getFen());

                // set found to true
                found = true;
                break;
            }
        }

        // if move was not found, throw error
        if (!found)
        {
            std::cout << "Move not found: " << algebraicMove << std::endl;
            throw std::exception();
        }
    }

    // return fen strings
    return fenStrings;
}