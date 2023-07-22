#pragma once
#include "game.h"
#include <vector>
#include <string>
#include <unordered_map>
using namespace std;

// method for splitting string
vector<string> split(const string& s, char delimiter);

// method to read and preprocess a PGN file into separate games (represented by a vector of strings)
vector<vector<string>> processPGN(string filename, int limit=5000);

// method to search through a list of moves given a piece and seeing if a certain piece type can move to a certain square with different pieces
int ambiguousMove(Board& board, Move moves[], int moveCount, Piece piece, int square);

// method to convert game result ("1-0", "0-1", "1/2-1/2") into a number (1, 0, 0.5)
string processResult(string result);

// method to convert a vector of strings (game) into a vector of fen strings
vector<string> processGame(vector<string> game, string startFen);

// method to convert a vector of strings (game) into a vector of fen strings with the move played per position
vector<string> processGameWithMoves(vector<string> game, string startFen, int limit);

// given a vector of fen+move, convert it into an unordered map of fen to all moves played from that position
unordered_map<string, vector<string>> fenMovesToMap(vector<string> fenMoves);