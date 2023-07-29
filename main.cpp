#include "bitboard.h"
#include "game.h"
#include "search.h"
#include "evaluate.h"
#include "searchboost.h"
#include "book.h"
#include "tuner.h"
#include "tables.h"
#include "evalparams.h"
#include "uci.h"
#include <iostream>
#include <string>
#include <chrono>
#include <fstream>
#include <cstring>
using namespace std;

int HISTORY_SIZE = 1;

int main()
{
	// r4k1r/1pP1npp1/1Pb4p/p6q/Q1p2p1b/B1P2N2/P2N2P1/R2K3R w - - 0 1 is a wonderful FEN string to test with
	// rn3rk1/pbppq1pp/1p2pb2/4N2Q/3PN3/3B4/PPP2PPP/R3K2R w KQ - 7 11 is a mate in 7 puzzle
	// position startpos moves d2d4 d7d5 b1c3 b8c6 c1f4 e7e5 f4e5 c6e5 d4e5 f8b4 d1d2 g8e7 e2e3 c8f5 f1d3 d8d7 a2a3 b4c3 d2c3 f5d3 c2d3 e7c6 g1f3 a7a5 d3d4 e8g8 c3c2 a5a4 h2h3 f7f6 e1c1 c6a5 e5f6 g7f6 f3h4 d7b5 h1e1 a5c4 e3e4 a8e8 e4d5 b5d5 h4f5 d5b5 f5e3 c4d6 c1b1 b5a5 f2f4 f8f7 c2c5 a5a6 e3g4 e8e1 d1e1 a6d3 b1a2 d3b3 a2a1 b3g3 c5c1 g8g7 c1e3 g3e3 g4e3 f7e7 e3c2 e7e1 c2e1 g7f7 e1c2 d6b5 d4d5 c7c6 d5c6 b7c6 a1a2 h7h5 b2b3 b5c3 a2b2 c3d1 b2b1 d1c3 b1b2
	// position startpos moves e2e4 c7c5 g1f3 e7e6 d2d4 c5d4 f3d4 b8c6 b1c3 d8c7 d4b5 c7b8 c1g5 a7a6 b5d4 e6e5 d4c6 b7c6 a1b1 h7h6 g5h4 f8b4 f1c4 b8d6 d1h5 g7g6 h5f3 h8h7 e1g1 b4c3 b2c3 d6c5 c4b3 a6a5 f1d1 a5a4 b3a4 g6g5 h4g3 a8a4 f3f5 h7g7 b1b8 f7f6 b8c8 e8f7 f5d7 g8e7 d7e8 f7e6 e8d7 e6f7 d7e8 f7e6
	// position fen 3kNr2/p2pppNq/Pr2p2p/8/8/8/1Q4B1/1R2b2K w - - 0 3
	// position fen r2q4/1pp5/p2b2Q1/P7/7k/4P3/1B1R2PP/4Kb2 w - - 4 35 with threads (5) gives mate score with bad d2f2 move

	// initialize sliding attacks
	processBishopAttacks("bishopAttackTable.txt");
	processRookAttacks("rookAttackTable.txt");

	// load parameters
	srand(time(NULL));
	loadParameters("best_parameters.txt");

	// load book

	// process pgn file
	vector<vector<string>> games = processPGN("book_games.pgn", 5000);

	// process games into fen strings and put into one large vector
	vector<string> bookFens;
	for (int i = 0; i < games.size(); i++)
	{
		vector<string> game = games[i];
		string startFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
		vector<string> fens = processGameWithMoves(game, startFen, 10);
		bookFens.insert(bookFens.end(), fens.begin(), fens.end());
	}

	// convert bookFens into a map
	unordered_map<string, vector<string>> bookMap = fenMovesToMap(bookFens);

	/*while (true) {
		loadParameters("best_parameters.txt");
		pso("positions.txt", 0.5, 50, 0.8, 0.1, 0.1, 1);
	}*/

	std::cout << "Welcome to the Chess Engine!" << std::endl;
	HISTORY_SIZE = 64;

	uci(bookMap);

	return 0;
}