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

int main()
{
	// r4k1r/1pP1npp1/1Pb4p/p6q/Q1p2p1b/B1P2N2/P2N2P1/R2K3R w - - 0 1 is a wonderful FEN string to test with
	// rn3rk1/pbppq1pp/1p2pb2/4N2Q/3PN3/3B4/PPP2PPP/R3K2R w KQ - 7 11 is a mate in 7 puzzle

	// initialize sliding attacks
	processBishopAttacks("bishopAttackTable.txt");
	processRookAttacks("rookAttackTable.txt");

	// load parameters
	loadParameters("best_parameters2.txt");
	srand(time(NULL));
	//pso("positions.txt", 0.5, 100, 0.8, 0.1, 0.1, 1);

	uci();

	return 0;
}