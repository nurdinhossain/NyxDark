#include "bitboard.h"
#include "game.h"
#include "tables.h"
#include <iostream>
#include <cmath>

const int MAX_MOVES = 256;

Color extractColor(int fullPiece)
{
    // checks if the bit in the 4th position is 1 --> black
    return (fullPiece & 0b1000) ? Color::BLACK : Color::WHITE;
}

Piece extractPiece(int fullPiece)
{
    // checks the next 3 bits to see what piece it is
    return (Piece)(fullPiece & 0b0111);
}

Square squareToIndex(std::string square)
{
    // converts a square to an index - h1 --> 0, a8 --> 63
    int file = 7 - (square[0] - 'a');
    int rank = square[1] - '1';
    return (Square)(rank * 8 + file);
}

std::string indexToSquare(Square index)
{
    // converts an index to a square - 0 --> h1, 63 --> a8
    int file = 7 - (index % 8);
    int rank = index / 8;
    return std::string(1, 'a' + file) + std::to_string(rank + 1);
}

// given an index from white's perspective, return the index from whichever color's perspective
// indices for black have the same col in the array as white but the row is 7 - row
int getTableIndex(int index, Color color)
{
    if (color == Color::WHITE) return index;
    int col = index % 8;
    int row = index / 8;
    return (7 - row) * 8 + col;
}

Board::Board(std::string fen) 
{   
    /* INITIALIZE FIELDS */

    // FOR BOARD STATE
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 6; j++) {
            pieces[i][j] = 0;
        }
    }

    occupied[0] = 0;
    occupied[1] = 0;
    fullOccupied = 0;

    for (int i = 0; i < 64; i++) {
        squareToPiece[i] = Piece::EMPTY;
    }

    // FOR STATIC EVALUATION
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 6; j++) {
            pieceCounts[i][j] = 0;
        }
    }

    for (int i = 0; i < 2; i++) {
        material[i] = 0;
    }

    staticEvalOpening = 0;
    staticEvalEndgame = 0;

    // FOR GENERATING LEGAL MOVES
    pinnedPiecesMG = 0;
    checkers = 0;
    occMG = 0;
    blockMaskMG = 0;

    // split string into parts by space
    std::string parts[4];
    for (int i = 0; i < 4; i++) {
        parts[i] = fen.substr(0, fen.find(" "));
        fen = fen.substr(fen.find(" ") + 1);
    }
    std::string board = parts[0], turn = parts[1], castling = parts[2], enpassant = parts[3];

    // set up board
    int index = 63;
    for (char c : board) {
        if (c == '/') continue;
        if (c >= '1' && c <= '8') {
            index -= c - '0';
        } else {
            // get piece color
            Color color = (c >= 'a' && c <= 'z') ? Color::BLACK : Color::WHITE;

            // get piece type
            Piece piece;
            switch (c) {
                case 'p':
                case 'P':
                    piece = Piece::PAWN;
                    break;
                case 'r':
                case 'R':
                    piece = Piece::ROOK;
                    break;
                case 'n':
                case 'N':
                    piece = Piece::KNIGHT;
                    break;
                case 'b':
                case 'B':
                    piece = Piece::BISHOP;
                    break;
                case 'q':
                case 'Q':
                    piece = Piece::QUEEN;
                    break;
                case 'k':
                    kingIndices[1] = static_cast<Square>(index);
                    piece = Piece::KING;
                    break;
                case 'K':
                    kingIndices[0] = static_cast<Square>(index);
                    piece = Piece::KING;
                    break;
            }

            // update piece counts
            pieceCounts[color][piece-1]++;

            // update static eval
            int tableIndex = getTableIndex(index, color);
            staticEvalOpening += TABLES[piece-1][0][tableIndex] * (color == Color::WHITE ? 1 : -1);
            staticEvalEndgame += TABLES[piece-1][1][tableIndex] * (color == Color::WHITE ? 1 : -1);
            material[color] += PIECE_VALUES[piece-1];

            // set piece
            pieces[color][piece-1] |= (1ULL << index);
            squareToPiece[index] = static_cast<int>(piece) + 8 * static_cast<int>(color);
            occupied[color] |= (1ULL << index);
            fullOccupied |= (1ULL << index);
            index--;
        }
    }

    // set up castling rights and other stuff
    nextMove = (turn == "w") ? Color::WHITE : Color::BLACK;
    castlingRights = 0;
    if (castling.find("K") != std::string::npos) castlingRights |= WHITE_KING_SIDE;
    if (castling.find("Q") != std::string::npos) castlingRights |= WHITE_QUEEN_SIDE;
    if (castling.find("k") != std::string::npos) castlingRights |= BLACK_KING_SIDE;
    if (castling.find("q") != std::string::npos) castlingRights |= BLACK_QUEEN_SIDE;
    if (enpassant != "-") enPassant = squareToIndex(enpassant);
    else enPassant = Square::NONE;

    // set up hash
    zobristHash();

    // add to history
    history.store(currentHash, 1);
}

Board::~Board()
{
}

// clone
Board* Board::clone() const
{
    Board* clone = new Board();
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 6; j++) {
            clone->pieces[i][j] = pieces[i][j];
        }
    }
    clone->occupied[0] = occupied[0];
    clone->occupied[1] = occupied[1];
    clone->fullOccupied = fullOccupied;
    for (int i = 0; i < 64; i++) {
        clone->squareToPiece[i] = squareToPiece[i];
    }
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 6; j++) {
            clone->pieceCounts[i][j] = pieceCounts[i][j];
        }
    }
    for (int i = 0; i < 2; i++) {
        clone->material[i] = material[i];
    }
    clone->staticEvalOpening = staticEvalOpening;
    clone->staticEvalEndgame = staticEvalEndgame;
    clone->nextMove = nextMove;
    clone->castlingRights = castlingRights;
    clone->enPassant = enPassant;
    clone->currentHash = currentHash;

    // clone history
    clone->history = *(history.clone());

    clone->pinnedPiecesMG = pinnedPiecesMG;
    clone->checkers = checkers;
    clone->occMG = occMG;
    clone->blockMaskMG = blockMaskMG;
    return clone;
}

void Board::print() const
{
    std::cout << "  +---+---+---+---+---+---+---+---+" << std::endl;
    for (int row = 7; row >= 0; row--)
    {
        std::cout << row + 1 << " | ";
        for (int col = 7; col >= 0; col--)
        {
            int index = row * 8 + col;
            int piece = squareToPiece[index];
            if (piece == Piece::EMPTY) {
                std::cout << " ";
            } else {
                Color color = extractColor(piece);
                Piece type = extractPiece(piece);
                std::string pieceString;
                switch (type) {
                    case Piece::PAWN:
                        pieceString = "P";
                        break;
                    case Piece::ROOK:
                        pieceString = "R";
                        break;
                    case Piece::KNIGHT:
                        pieceString = "N";
                        break;
                    case Piece::BISHOP:
                        pieceString = "B";
                        break;
                    case Piece::QUEEN:
                        pieceString = "Q";
                        break;
                    case Piece::KING:
                        pieceString = "K";
                        break;
                }
                // if white, keep, if black, lowercase
                if (color == Color::BLACK) pieceString[0] += 32;
                std::cout << pieceString;
            }
            std::cout << " | ";
        }
        std::cout << std::endl;
        std::cout << "  +---+---+---+---+---+---+---+---+" << std::endl;
    }
    std::cout << "    a   b   c   d   e   f   g   h";
    std::cout << std::endl;
}

void Board::zobristHash()
{
    currentHash = 0;

    // hash for pieces
    for (int i = 0; i < 64; i++) {
        int piece = squareToPiece[i];
        if (piece != Piece::EMPTY) {
            Color color = extractColor(piece);
            Piece type = extractPiece(piece);
            currentHash ^= ZOBRIST_PIECES[color][type-1][i];

            // pawn hash
            if (type == PAWN || type == KING)
            {
                pawnHash ^= ZOBRIST_PIECES[color][type-1][i];
            }
        }
    }

    // hash for castling rights
    currentHash ^= ZOBRIST_CASTLE[castlingRights];

    // hash for en passant
    if (enPassant != Square::NONE) {
        currentHash ^= ZOBRIST_EN_PASSANT[enPassant % 8];
    }

    // hash for turn
    if (nextMove == Color::BLACK) {
        currentHash ^= ZOBRIST_SIDE;
    }
}

/* GETTERS */
std::string Board::getFen() const
{
    std::string fen = "";
    int emptyCount = 0;
    for (int row = 7; row >= 0; row--)
    {
        for (int col = 7; col >= 0; col--)
        {
            int index = row * 8 + col;
            int piece = squareToPiece[index];
            if (piece == Piece::EMPTY) {
                emptyCount++;
            } else {
                if (emptyCount > 0) {
                    fen += std::to_string(emptyCount);
                    emptyCount = 0;
                }
                Color color = extractColor(piece);
                Piece type = extractPiece(piece);
                std::string pieceString;
                switch (type) {
                    case Piece::PAWN:
                        pieceString = "P";
                        break;
                    case Piece::ROOK:
                        pieceString = "R";
                        break;
                    case Piece::KNIGHT:
                        pieceString = "N";
                        break;
                    case Piece::BISHOP:
                        pieceString = "B";
                        break;
                    case Piece::QUEEN:
                        pieceString = "Q";
                        break;
                    case Piece::KING:
                        pieceString = "K";
                        break;
                }
                // if white, keep, if black, lowercase
                if (color == Color::BLACK) pieceString[0] += 32;
                fen += pieceString;
            }
        }
        if (emptyCount > 0) {
            fen += std::to_string(emptyCount);
            emptyCount = 0;
        }
        if (row != 0) fen += "/";
    }

    fen += " ";
    fen += (nextMove == Color::WHITE) ? "w" : "b";

    fen += " ";
    if (castlingRights == 0) fen += "-";
    else {
        if (castlingRights & WHITE_KING_SIDE) fen += "K";
        if (castlingRights & WHITE_QUEEN_SIDE) fen += "Q";
        if (castlingRights & BLACK_KING_SIDE) fen += "k";
        if (castlingRights & BLACK_QUEEN_SIDE) fen += "q";
    }

    fen += " ";
    if (enPassant == Square::NONE) fen += "-";
    else fen += indexToSquare(enPassant);

    fen += " ";
    fen += "0 1";

    return fen;
}

UInt64 Board::getPiece(Color color, Piece piece) const
{
    return pieces[color][piece-1];
}

UInt64 Board::getOccupied(Color color) const
{
    return occupied[color];
}

UInt64 Board::getFullOccupied() const
{
    return fullOccupied;
}

int Board::getSquareToPiece(Square square) const
{
    return squareToPiece[square];
}

int Board::getCastlingRights() const
{
    return castlingRights;
}

Color Board::getNextMove() const
{
    return nextMove;
}

Square Board::getEnPassant() const
{
    return enPassant;
}

UInt64 Board::getPawnHash() const
{
    return pawnHash;
}

UInt64 Board::getCurrentHash() const
{
    return currentHash;
}

int Board::getHashCount(UInt64 index)
{
    int value;
    if (history.lookup(index, value)) return value;
    return 0;
}

int Board::getPieceCount(Color color, Piece piece) const
{
    return pieceCounts[color][piece-1];
}

int Board::getMaterial(Color color) const
{
    return material[color];
}

int Board::getStaticEvalOpening() const
{
    return staticEvalOpening;
}

int Board::getStaticEvalEndgame() const
{
    return staticEvalEndgame;
}

int Board::getPhase() const
{
    int phase = Phase::TOTAL_PHASE;
    phase -= Phase::PAWN_PHASE * (getPieceCount(Color::WHITE, Piece::PAWN) + getPieceCount(Color::BLACK, Piece::PAWN));
    phase -= Phase::KNIGHT_PHASE * (getPieceCount(Color::WHITE, Piece::KNIGHT) + getPieceCount(Color::BLACK, Piece::KNIGHT));
    phase -= Phase::BISHOP_PHASE * (getPieceCount(Color::WHITE, Piece::BISHOP) + getPieceCount(Color::BLACK, Piece::BISHOP));
    phase -= Phase::ROOK_PHASE * (getPieceCount(Color::WHITE, Piece::ROOK) + getPieceCount(Color::BLACK, Piece::ROOK));
    phase -= Phase::QUEEN_PHASE * (getPieceCount(Color::WHITE, Piece::QUEEN) + getPieceCount(Color::BLACK, Piece::QUEEN));
    
    return (phase * 256 + Phase::TOTAL_PHASE / 2) / Phase::TOTAL_PHASE;
}

bool Board::insufficientMaterial(Color color) const
{
    // if there are any pawns, rooks, or queens, there is sufficient material
    if (getPieceCount(color, Piece::PAWN) > 0 || getPieceCount(color, Piece::ROOK) > 0 || getPieceCount(color, Piece::QUEEN) > 0) return false;

    // if there are at least two bishops or a bishop and a knight, there is sufficient material
    if (getPieceCount(color, Piece::BISHOP) + getPieceCount(color, Piece::KNIGHT) >= 2 && getPieceCount(color, Piece::BISHOP) > 0) return false;

    // if there are more than two knights, there is sufficient material
    if (getPieceCount(color, Piece::KNIGHT) > 2) return false;

    return true;
}

UInt64 Board::getCheckers() const
{
    return checkers;
}

/* SEMI-LEGAL MOVE GENERATION HELPERS */
UInt64 Board::inCheck(UInt64 bishopAttackFromKing, UInt64 rookAttackFromKing) const
{
    int enemy = Color::BLACK - nextMove;
    Square kingIndex = kingIndices[nextMove];

    // static (non-sliding) attacks
    UInt64 knightAttack = KNIGHT_ATTACKS[kingIndex] & pieces[enemy][Piece::KNIGHT-1];
    UInt64 pawnAttack = PAWN_ATTACKS[nextMove][kingIndex] & pieces[enemy][Piece::PAWN-1];
    UInt64 kingAttack = KING_ATTACKS[kingIndex] & pieces[enemy][Piece::KING-1];

    // sliding attacks
    UInt64 bishopAttack = bishopAttackFromKing & (pieces[enemy][Piece::BISHOP-1] | pieces[enemy][Piece::QUEEN-1]);
    UInt64 rookAttack = rookAttackFromKing & (pieces[enemy][Piece::ROOK-1] | pieces[enemy][Piece::QUEEN-1]);

    return knightAttack | pawnAttack | kingAttack | bishopAttack | rookAttack;
}

UInt64 Board::inCheckFull() const
{   
    Square kingIndex = kingIndices[nextMove];
    UInt64 occupancy = fullOccupied ^ pieces[nextMove][Piece::KING-1];

    UInt64 bishopAttackFromKing = lookupBishopAttack(kingIndex, occupancy);
    UInt64 rookAttackFromKing = lookupRookAttack(kingIndex, occupancy);

    return inCheck(bishopAttackFromKing, rookAttackFromKing);
}

UInt64 Board::getAttackersForSquare(Square square) const
{
    UInt64 occupancy = fullOccupied ^ (1ULL << square);
    UInt64 bishopAttack = lookupBishopAttack(square, occupancy) & (pieces[Color::WHITE][Piece::BISHOP-1] | pieces[Color::WHITE][Piece::QUEEN-1] | pieces[Color::BLACK][Piece::BISHOP-1] | pieces[Color::BLACK][Piece::QUEEN-1]);
    UInt64 rookAttack = lookupRookAttack(square, occupancy) & (pieces[Color::WHITE][Piece::ROOK-1] | pieces[Color::WHITE][Piece::QUEEN-1] | pieces[Color::BLACK][Piece::ROOK-1] | pieces[Color::BLACK][Piece::QUEEN-1]);
    UInt64 knightAttack = KNIGHT_ATTACKS[square] & (pieces[Color::WHITE][Piece::KNIGHT-1] | pieces[Color::BLACK][Piece::KNIGHT-1]);
    UInt64 pawnAttack = PAWN_ATTACKS[Color::WHITE][square] & pieces[Color::BLACK][Piece::PAWN-1];
    pawnAttack |= PAWN_ATTACKS[Color::BLACK][square] & pieces[Color::WHITE][Piece::PAWN-1];
    UInt64 kingAttack = KING_ATTACKS[square] & (pieces[Color::WHITE][Piece::KING-1] | pieces[Color::BLACK][Piece::KING-1]);

    return bishopAttack | rookAttack | knightAttack | pawnAttack | kingAttack;
}

UInt64 Board::checkBlockMask(UInt64 bishopAttackFromKing, UInt64 rookAttackFromKing) const
{
    UInt64 blockMask = 0ULL;

    // extract some information about the checking piece
    int checkerIndex = lsb(checkers);
    Piece checkerPiece = extractPiece(squareToPiece[checkerIndex]);

    // bishop, rook, or queen
    if (checkerPiece == Piece::BISHOP)
    {
        blockMask = lookupBishopAttack(checkerIndex, fullOccupied ^ (1ULL << checkerIndex)) & bishopAttackFromKing;
    }
    else if (checkerPiece == Piece::ROOK)
    {
        blockMask = lookupRookAttack(checkerIndex, fullOccupied ^ (1ULL << checkerIndex)) & rookAttackFromKing;
    }
    else if (checkerPiece == Piece::QUEEN)
    {
        int checkerRank = checkerIndex / 8, checkerFile = checkerIndex % 8;
        int kingRank = kingIndices[nextMove] / 8, kingFile = kingIndices[nextMove] % 8;

        if (checkerRank == kingRank || checkerFile == kingFile)
        {
            blockMask = lookupRookAttack(checkerIndex, fullOccupied ^ (1ULL << checkerIndex)) & rookAttackFromKing;
        }
        else
        {
            blockMask = lookupBishopAttack(checkerIndex, fullOccupied ^ (1ULL << checkerIndex)) & bishopAttackFromKing;
        }
    }

    return blockMask;
}

UInt64 Board::pinned(UInt64 bishopAttackFromKing, UInt64 rookAttackFromKing) const
{
    UInt64 pinned = 0ULL;
    int enemy = Color::BLACK - nextMove;
    UInt64 enemyRooks = (pieces[enemy][Piece::ROOK-1] | pieces[enemy][Piece::QUEEN-1]) & (RANK_MASKS[kingIndices[nextMove] / 8] | FILE_MASKS[kingIndices[nextMove] % 8]);
    UInt64 enemyBishops = (pieces[enemy][Piece::BISHOP-1] | pieces[enemy][Piece::QUEEN-1]) & (FORWARD_DIAG_MASKS[FORWARD_DIAG_INDEX[kingIndices[nextMove]]] | BACKWARD_DIAG_MASKS[BACKWARD_DIAG_INDEX[kingIndices[nextMove]]]);

    UInt64 intersection = 0ULL;
    while (enemyRooks)
    {
        int index = lsb(enemyRooks);
        
        // find rook attack path and intersection
        UInt64 rookAttack = lookupRookAttack(index, fullOccupied ^ (1ULL << index));
        intersection = rookAttack & rookAttackFromKing;

        // make sure its a pinned piece and not a checking piece
        int pinnedIndex = lsb(intersection);
        if (pinnedIndex < 64)
        {
            if (squareToPiece[pinnedIndex] != Piece::EMPTY)
            {
                pinned |= intersection;
            }
        }

        enemyRooks ^= (1ULL << index);
    }

    while (enemyBishops)
    {
        int index = lsb(enemyBishops);

        // find bishop attack path and intersection
        UInt64 bishopAttack = lookupBishopAttack(index, fullOccupied ^ (1ULL << index));
        intersection = bishopAttack & bishopAttackFromKing;

        // make sure its a pinned piece and not a checking piece
        int pinnedIndex = lsb(intersection);
        if (pinnedIndex < 64)
        {
            if (squareToPiece[pinnedIndex] != Piece::EMPTY)
            {
                pinned |= intersection;
            }
        }

        enemyBishops ^= (1ULL << index);
    }

    return pinned;
}

UInt64 Board::pinnedLegalMask(Square pinnedIndex) const
{
    Square kingIndex = kingIndices[nextMove];
    int kingRow = kingIndex / 8, kingCol = kingIndex % 8;
    int pinnedRow = pinnedIndex / 8, pinnedCol = pinnedIndex % 8;

    if (pinnedRow == kingRow) return RANK_MASKS[pinnedRow];
    else if (pinnedCol == kingCol) return FILE_MASKS[pinnedCol];
    else if ((pinnedRow > kingRow && pinnedCol > kingCol) || (pinnedRow < kingRow && pinnedCol < kingCol)) return BACKWARD_DIAG_MASKS[BACKWARD_DIAG_INDEX[kingIndex]];
    else return FORWARD_DIAG_MASKS[FORWARD_DIAG_INDEX[kingIndex]];
}

/* SEMI-LEGAL MOVE GENERATION */
UInt64 Board::pawnPushBoard(Square square) const
{
    UInt64 rawPushes = PAWN_PUSHES[nextMove][square];

    // remove pushes that are blocked
    int squareInFront = (nextMove == Color::WHITE) ? square + 8 : square - 8;
    if (squareToPiece[squareInFront] != Piece::EMPTY) return 0ULL;
    if (popCount(rawPushes) == 2)
    {
        int squareTwoInFront = (nextMove == Color::WHITE) ? square + 16 : square - 16;
        if (squareToPiece[squareTwoInFront] != Piece::EMPTY) return rawPushes & ~fullOccupied;
    }

    return rawPushes;
}

UInt64 Board::pawnAttackBoard(Square square) const
{
    return PAWN_ATTACKS[nextMove][square] & occupied[1 - nextMove];
}

UInt64 Board::knightAttackBoard(Square square) const
{
    return KNIGHT_ATTACKS[square] & ~occupied[nextMove];
}

UInt64 Board::bishopAttackBoard(Square square) const
{
    return lookupBishopAttack(square, fullOccupied ^ (1ULL << square)) & ~occupied[nextMove];
}

UInt64 Board::rookAttackBoard(Square square) const
{
    return lookupRookAttack(square, fullOccupied ^ (1ULL << square)) & ~occupied[nextMove];
}

UInt64 Board::queenAttackBoard(Square square) const
{
    return lookupQueenAttack(square, fullOccupied ^ (1ULL << square)) & ~occupied[nextMove];
}

UInt64 Board::kingAttackBoard(Square square) const
{
    return KING_ATTACKS[square] & ~occupied[nextMove];
}

void Board::moveGenerationSetup()
{
    // see if we're in check
    UInt64 occupancy = fullOccupied ^ pieces[nextMove][Piece::KING-1];
    UInt64 bishopAttackFromKing = lookupBishopAttack(kingIndices[nextMove], occupancy);
    UInt64 rookAttackFromKing = lookupRookAttack(kingIndices[nextMove], occupancy);

    // find all checking pieces
    checkers = inCheck(bishopAttackFromKing, rookAttackFromKing);
    int numCheckers = popCount(checkers);
    occMG = occupied[nextMove];
    blockMaskMG = FILLED_BOARD;

    // if we're in double check, we can only move the king, so set the occupancy to just the king
    if (numCheckers > 1)
    {
        occMG = pieces[nextMove][Piece::KING-1];
    } 
    // if we're in single check, we can only move the king or block the check
    else if (numCheckers == 1) 
    {
        blockMaskMG = checkBlockMask(bishopAttackFromKing, rookAttackFromKing) | checkers;
        pinnedPiecesMG = pinned(bishopAttackFromKing, rookAttackFromKing);
    }
    else pinnedPiecesMG = pinned(bishopAttackFromKing, rookAttackFromKing);
}

void Board::generateMoves(Move moves[], int& moveCount, bool attackOnly, bool quietOnly) 
{
    /* ASSUMES THAT THE MOVE GENERATION SETUP HAS ALREADY BEEN RUN */
    UInt64 occ = occMG;
    UInt64 blockMask = blockMaskMG;
    UInt64 pinnedPieces = pinnedPiecesMG;

    UInt64 moveBoard = 0ULL;
    while (occ)
    {
        // piece index
        Square index = static_cast<Square>(lsb(occ));

        // extract necessary information
        Piece piece = extractPiece(squareToPiece[index]);

        // retrieve the move board
        switch (piece)
        {
            case Piece::PAWN:
                moveBoard = (pawnPushBoard(index) | pawnAttackBoard(index)) & blockMask;
                break;
            case Piece::KNIGHT:
                moveBoard = knightAttackBoard(index) & blockMask;
                break;
            case Piece::BISHOP:
                moveBoard = bishopAttackBoard(index) & blockMask;
                break;
            case Piece::ROOK:
                moveBoard = rookAttackBoard(index) & blockMask;
                break;
            case Piece::QUEEN:
                moveBoard = queenAttackBoard(index) & blockMask;
                break;
            case Piece::KING:
                moveBoard = kingAttackBoard(index);
                break;
        }

        // bitwise AND with the opposite color's occupancy if we're only looking for attacks
        if (attackOnly)
        {
            UInt64 attackOccupancy = occupied[1 - nextMove];
            // if piece board is pawn board, extract promotions along with attacks
            if (piece == Piece::PAWN)
            {
                UInt64 promoMask = RANK_MASKS[7 - nextMove * 7];
                attackOccupancy |= promoMask;
            }

            moveBoard &= attackOccupancy;
        }

        // bitwise AND with NOT the opposite color's occupancy if we're only looking for quiet moves
        if (quietOnly)
        {
            UInt64 quietOccupancy = ~occupied[1 - nextMove];
            // if piece board is pawn board, DON'T extract promotions along with attacks
            if (piece == Piece::PAWN)
            {
                UInt64 promoMask = RANK_MASKS[7 - nextMove * 7];
                quietOccupancy &= ~promoMask;
            }

            moveBoard &= quietOccupancy;
        }

        // pinned pieces
        if (pinnedPieces & (1ULL << index))
        {
            moveBoard &= pinnedLegalMask(index);
        }

        // add the moves to the move list
        while (moveBoard)
        {
            Square to = static_cast<Square>(lsb(moveBoard));
            Piece capturedPiece = extractPiece(squareToPiece[to]);
            MoveType flag = (capturedPiece == Piece::EMPTY ? MoveType::QUIET : MoveType::CAPTURE);

            // deal with pawn pushes
            if (piece == Piece::PAWN)
            {
                int rank = to / 8;
                if (abs(to - index) == 16) flag = MoveType::DOUBLE_PAWN_PUSH;
                if (rank == 0 || rank == 7)
                {
                    // PROMOTION
                    if (flag == MoveType::CAPTURE)
                    {
                        moves[moveCount++] = { MoveType::KNIGHT_PROMOTION_CAPTURE, index, to, capturedPiece, castlingRights, enPassant };
                        moves[moveCount++] = { MoveType::BISHOP_PROMOTION_CAPTURE, index, to, capturedPiece, castlingRights, enPassant };
                        moves[moveCount++] = { MoveType::ROOK_PROMOTION_CAPTURE, index, to, capturedPiece, castlingRights, enPassant };
                        moves[moveCount++] = { MoveType::QUEEN_PROMOTION_CAPTURE, index, to, capturedPiece, castlingRights, enPassant };
                    }
                    else
                    {
                        moves[moveCount++] = { MoveType::KNIGHT_PROMOTION, index, to, capturedPiece, castlingRights, enPassant };
                        moves[moveCount++] = { MoveType::BISHOP_PROMOTION, index, to, capturedPiece, castlingRights, enPassant };
                        moves[moveCount++] = { MoveType::ROOK_PROMOTION, index, to, capturedPiece, castlingRights, enPassant };
                        moves[moveCount++] = { MoveType::QUEEN_PROMOTION, index, to, capturedPiece, castlingRights, enPassant };
                    }
                }
                else moves[moveCount++] = { flag, index, to, capturedPiece, castlingRights, enPassant };
            }   

            // deal with king moves
            else if (piece == Piece::KING)
            {
                if (isKingMoveLegal(index, to)) moves[moveCount++] = { flag, index, to, capturedPiece, castlingRights, enPassant };
            }
            else moves[moveCount++] = { flag, index, to, capturedPiece, castlingRights, enPassant };

            // remove the move from the move board
            moveBoard &= moveBoard - 1;
        }

        // remove the piece from the occupancy
        occ &= occ - 1;
    }

    // castling
    if (checkers == 0 && !quietOnly)
    {
        if (nextMove == Color::WHITE)
        {
            // king side castle
            if (castlingRights & Castle::WHITE_KING_SIDE)
            {
                // check if squares are empty
                if ((fullOccupied & CastleThreat::WHITE_KINGSIDE_THREAT) == 0)
                {
                    // check if squared are not attacked
                    Square oldKingIndex = kingIndices[Color::WHITE];
                    kingIndices[Color::WHITE] = Square::f1;
                    UInt64 attackers = inCheck(lookupBishopAttack(Square::f1, fullOccupied), lookupRookAttack(Square::f1, fullOccupied));
                    kingIndices[Color::WHITE] = Square::g1;
                    attackers |= inCheck(lookupBishopAttack(Square::g1, fullOccupied), lookupRookAttack(Square::g1, fullOccupied));
                    kingIndices[Color::WHITE] = oldKingIndex;

                    if (attackers == 0)
                    {
                        moves[moveCount++] = { MoveType::KING_CASTLE, Square::e1, Square::g1, Piece::EMPTY, castlingRights, enPassant };
                    }
                }
            }

            // queen side castle
            if (castlingRights & Castle::WHITE_QUEEN_SIDE)
            {
                // check if squares are empty
                if ((fullOccupied & CastleThreat::WHITE_QUEENSIDE_THREAT) == 0)
                {
                    // check if squared are not attacked
                    Square oldKingIndex = kingIndices[Color::WHITE];
                    kingIndices[Color::WHITE] = Square::d1;
                    UInt64 attackers = inCheck(lookupBishopAttack(Square::d1, fullOccupied), lookupRookAttack(Square::d1, fullOccupied));
                    kingIndices[Color::WHITE] = Square::c1;
                    attackers |= inCheck(lookupBishopAttack(Square::c1, fullOccupied), lookupRookAttack(Square::c1, fullOccupied));
                    kingIndices[Color::WHITE] = oldKingIndex;

                    if (attackers == 0)
                    {
                        moves[moveCount++] = { MoveType::QUEEN_CASTLE, Square::e1, Square::c1, Piece::EMPTY, castlingRights, enPassant };
                    }
                }
            }
        }
        else
        {
            // king side castle
            if (castlingRights & Castle::BLACK_KING_SIDE)
            {
                // check if squares are empty
                if ((fullOccupied & CastleThreat::BLACK_KINGSIDE_THREAT) == 0)
                {
                    // check if squared are not attacked
                    Square oldKingIndex = kingIndices[Color::BLACK];
                    kingIndices[Color::BLACK] = Square::f8;
                    UInt64 attackers = inCheck(lookupBishopAttack(Square::f8, fullOccupied), lookupRookAttack(Square::f8, fullOccupied));
                    kingIndices[Color::BLACK] = Square::g8;
                    attackers |= inCheck(lookupBishopAttack(Square::g8, fullOccupied), lookupRookAttack(Square::g8, fullOccupied));
                    kingIndices[Color::BLACK] = oldKingIndex;

                    if (attackers == 0)
                    {
                        moves[moveCount++] = { MoveType::KING_CASTLE, Square::e8, Square::g8, Piece::EMPTY, castlingRights, enPassant };
                    }
                }
            }

            // queen side castle
            if (castlingRights & Castle::BLACK_QUEEN_SIDE)
            {
                // check if squares are empty
                if ((fullOccupied & CastleThreat::BLACK_QUEENSIDE_THREAT) == 0)
                {
                    // check if squared are not attacked
                    Square oldKingIndex = kingIndices[Color::BLACK];
                    kingIndices[Color::BLACK] = Square::d8;
                    UInt64 attackers = inCheck(lookupBishopAttack(Square::d8, fullOccupied), lookupRookAttack(Square::d8, fullOccupied));
                    kingIndices[Color::BLACK] = Square::c8;
                    attackers |= inCheck(lookupBishopAttack(Square::c8, fullOccupied), lookupRookAttack(Square::c8, fullOccupied));
                    kingIndices[Color::BLACK] = oldKingIndex;

                    if (attackers == 0)
                    {
                        moves[moveCount++] = { MoveType::QUEEN_CASTLE, Square::e8, Square::c8, Piece::EMPTY, castlingRights, enPassant };
                    }
                }
            }
        }
    }

    // en passant
    if (enPassant != Square::NONE && !quietOnly)
    {
        if (nextMove == Color::WHITE)
        {
            Square potentialAttackerOne = static_cast<Square>(enPassant - 9), potentialAttackerTwo = static_cast<Square>(enPassant - 7);
            if ((potentialAttackerOne % 8 == enPassant % 8 - 1) && squareToPiece[potentialAttackerOne] == Piece::PAWN)
            {
                if (isEPLegal(potentialAttackerOne, enPassant)) moves[moveCount++] = { MoveType::EN_PASSANT, potentialAttackerOne, enPassant, Piece::PAWN, castlingRights, enPassant };
            }
            if ((potentialAttackerTwo % 8 == enPassant % 8 + 1) && squareToPiece[potentialAttackerTwo] == Piece::PAWN)
            {
                if (isEPLegal(potentialAttackerTwo, enPassant)) moves[moveCount++] = { MoveType::EN_PASSANT, potentialAttackerTwo, enPassant, Piece::PAWN, castlingRights, enPassant };
            }
        }
        else
        {
            Square potentialAttackerOne = static_cast<Square>(enPassant + 7), potentialAttackerTwo = static_cast<Square>(enPassant + 9);
            if ((potentialAttackerOne % 8 == enPassant % 8 - 1) && extractPiece(squareToPiece[potentialAttackerOne]) == Piece::PAWN && extractColor(squareToPiece[potentialAttackerOne]) == Color::BLACK)
            {
                if (isEPLegal(potentialAttackerOne, enPassant)) moves[moveCount++] = { MoveType::EN_PASSANT, potentialAttackerOne, enPassant, Piece::PAWN, castlingRights, enPassant };
            }
            if ((potentialAttackerTwo % 8 == enPassant % 8 + 1) && extractPiece(squareToPiece[potentialAttackerTwo]) == Piece::PAWN && extractColor(squareToPiece[potentialAttackerTwo]) == Color::BLACK)
            {
                if (isEPLegal(potentialAttackerTwo, enPassant)) moves[moveCount++] = { MoveType::EN_PASSANT, potentialAttackerTwo, enPassant, Piece::PAWN, castlingRights, enPassant };
            }
        }
    }
}

bool Board::isEPLegal(Square from, Square to)
{
    UInt64 occupancy = (fullOccupied ^ (1ULL << from | 1ULL << to)) ^ pieces[nextMove][Piece::KING - 1];
    if (nextMove == Color::WHITE)
    {
        UInt64 takeBoard = (1ULL << (to - 8));
        occupancy ^= takeBoard;
        pieces[Color::BLACK][Piece::PAWN - 1] ^= takeBoard;

        // check if king is attacked
        UInt64 attackers = inCheck(lookupBishopAttack(kingIndices[Color::WHITE], occupancy), lookupRookAttack(kingIndices[Color::WHITE], occupancy));   
        pieces[Color::BLACK][Piece::PAWN - 1] ^= takeBoard;
        return attackers == 0;
    }
    else
    {
        UInt64 takeBoard = (1ULL << (to + 8));
        occupancy ^= takeBoard;
        pieces[Color::WHITE][Piece::PAWN - 1] ^= takeBoard;

        // check if king is attacked
        UInt64 attackers = inCheck(lookupBishopAttack(kingIndices[Color::BLACK], occupancy), lookupRookAttack(kingIndices[Color::BLACK], occupancy));
        pieces[Color::WHITE][Piece::PAWN - 1] ^= takeBoard;
        return attackers == 0;
    }
}

bool Board::isKingMoveLegal(Square from, Square to)
{
    UInt64 occupancy = (fullOccupied ^ (1ULL << from)) & ~(1ULL << to);

    // check if king is attacked
    Square oldKingIndex = kingIndices[nextMove];
    kingIndices[nextMove] = to;
    UInt64 attackers = inCheck(lookupBishopAttack(to, occupancy), lookupRookAttack(to, occupancy));
    kingIndices[nextMove] = oldKingIndex;
    return attackers == 0;
}

/* MAKING/UN-MAKING MOVES */
void Board::togglePiece(Color color, Piece piece, Square square)
{
    pieces[color][piece-1] ^= (1ULL << square);
    occupied[color] ^= (1ULL << square);
    fullOccupied ^= (1ULL << square);
    int tableIndex = getTableIndex(square, color);
    if (squareToPiece[square] == Piece::EMPTY) { // add piece
        squareToPiece[square] = static_cast<int>(piece) + 8 * static_cast<int>(color);
        pieceCounts[color][piece-1]++;
        material[color] += PIECE_VALUES[piece-1];
        staticEvalOpening += TABLES[piece-1][0][tableIndex] * (color == Color::WHITE ? 1 : -1);
        staticEvalEndgame += TABLES[piece-1][1][tableIndex] * (color == Color::WHITE ? 1 : -1);
    } else { // remove piece
        squareToPiece[square] = Piece::EMPTY;
        pieceCounts[color][piece-1]--;
        material[color] -= PIECE_VALUES[piece-1];
        staticEvalOpening -= TABLES[piece-1][0][tableIndex] * (color == Color::WHITE ? 1 : -1);
        staticEvalEndgame -= TABLES[piece-1][1][tableIndex] * (color == Color::WHITE ? 1 : -1);
    }

    currentHash ^= ZOBRIST_PIECES[color][piece-1][square];

    // update pawn hash
    /*if (piece == Piece::PAWN || piece == Piece::KING) {
        pawnHash ^= ZOBRIST_PIECES[color][piece-1][square];
    }*/
}

void Board::makeQuietMove(Square from, Square to)
{
    int fromPiece = squareToPiece[from];
    Piece piece = extractPiece(fromPiece);
    Color color = extractColor(fromPiece);

    // if from piece is king, update king index
    if (piece == Piece::KING) {
        kingIndices[color] = to;
    }

    togglePiece(color, piece, from);
    togglePiece(color, piece, to);
}

void Board::makeCaptureMove(Square from, Square to)
{
    int toPiece = squareToPiece[to];
    Piece capturedPiece = extractPiece(toPiece);
    Color capturedColor = extractColor(toPiece);

    togglePiece(capturedColor, capturedPiece, to);
    makeQuietMove(from, to);
}

void Board::makeEPMove(Square from, Square to)
{
    if (nextMove == Color::WHITE) {
        togglePiece(Color::BLACK, Piece::PAWN, static_cast<Square>(to - 8));
    } else {
        togglePiece(Color::WHITE, Piece::PAWN, static_cast<Square>(to + 8));
    }
    makeQuietMove(from, to);
}

void Board::castleKingSide(Color color) 
{
    if (color == Color::WHITE)
    {
        makeQuietMove(Square::e1, Square::g1);
        makeQuietMove(Square::h1, Square::f1);
    }
    else
    {
        makeQuietMove(Square::e8, Square::g8);
        makeQuietMove(Square::h8, Square::f8);
    }
}

void Board::castleQueenSide(Color color)
{
    if (color == Color::WHITE)
    {
        makeQuietMove(Square::e1, Square::c1);
        makeQuietMove(Square::a1, Square::d1);
    }
    else
    {
        makeQuietMove(Square::e8, Square::c8);
        makeQuietMove(Square::a8, Square::d8);
    }
}

void Board::makePromoMove(Square from, Square to, Piece promoPiece)
{
    int fromPiece = squareToPiece[from];
    Piece piece = extractPiece(fromPiece);

    togglePiece(nextMove, piece, from);
    togglePiece(nextMove, promoPiece, to);
}

void Board::makePromoCaptureMove(Square from, Square to, Piece promoPiece)
{
    int toPiece = squareToPiece[to];
    Piece capturedPiece = extractPiece(toPiece);
    Color capturedColor = extractColor(toPiece);

    togglePiece(capturedColor, capturedPiece, to);
    makePromoMove(from, to, promoPiece);
}

void Board::makeMove(Move &move)
{
    // deconstruct move
    MoveType type = move.type;
    Square from = move.from;
    Square to = move.to;
    Piece pieceTaken = move.pieceTaken;

    // undo old zobrists
    currentHash ^= ZOBRIST_CASTLE[castlingRights];
    if (enPassant != Square::NONE) currentHash ^= ZOBRIST_EN_PASSANT[enPassant % 8];

    // make move
    if (type == MoveType::QUIET) makeQuietMove(from, to);
    else if (type == MoveType::DOUBLE_PAWN_PUSH)
    {
        makeQuietMove(from, to);
        if (nextMove == Color::WHITE) enPassant = static_cast<Square>(to - 8);
        else enPassant = static_cast<Square>(to + 8);
    }
    else if (type == MoveType::KING_CASTLE)
    {
        castleKingSide(nextMove);
        if (nextMove == Color::WHITE) castlingRights &= 0b1100;
        else castlingRights &= 0b0011;
    }
    else if (type == MoveType::QUEEN_CASTLE)
    {
        castleQueenSide(nextMove);
        if (nextMove == Color::WHITE) castlingRights &= 0b1100;
        else castlingRights &= 0b0011;
    }
    else if (type == MoveType::CAPTURE) makeCaptureMove(from, to);
    else if (type == MoveType::EN_PASSANT) makeEPMove(from, to);
    else if (type <= MoveType::QUEEN_PROMOTION) makePromoMove(from, to, static_cast<Piece>(type - 4));
    else if (type <= MoveType::QUEEN_PROMOTION_CAPTURE) makePromoCaptureMove(from, to, static_cast<Piece>(type - 8));

    // update castling rights
    if (from == Square::h1 || to == Square::h1 || from == Square::e1) castlingRights &= 0b1110;
    if (from == Square::a1 || to == Square::a1 || from == Square::e1) castlingRights &= 0b1101;
    if (from == Square::h8 || to == Square::h8 || from == Square::e8) castlingRights &= 0b1011;
    if (from == Square::a8 || to == Square::a8 || from == Square::e8) castlingRights &= 0b0111;

    // update game state
    nextMove = static_cast<Color>(Color::BLACK - nextMove);

    // update en passant
    enPassant = (type != MoveType::DOUBLE_PAWN_PUSH) ? Square::NONE : enPassant;

    // update zobrist hash
    currentHash ^= ZOBRIST_CASTLE[castlingRights];
    if (enPassant != Square::NONE) currentHash ^= ZOBRIST_EN_PASSANT[enPassant % 8];
    currentHash ^= ZOBRIST_SIDE;

    // update history hash
    int value;
    if (history.lookup(currentHash, value)) history.store(currentHash, value + 1);
    else history.store(currentHash, 1);
}

void Board::unmakeMove(Move &move)
{
    // deconstruct move
    MoveType type = move.type;
    Square from = move.from;
    Square to = move.to;
    Piece pieceTaken = move.pieceTaken;
    int oldCastle = move.oldCastle;
    Square oldEP = move.oldEnPassant;
    Color color = static_cast<Color>(Color::BLACK - nextMove);

    // if position is new, remove it from history, otherwise decrement the count
    int value;
    if (history.lookup(currentHash, value)) {
        if (value == 1) history.remove(currentHash);
        else history.store(currentHash, value - 1);
    }
    
    // undo old zobrists
    currentHash ^= ZOBRIST_CASTLE[castlingRights];
    if (enPassant != Square::NONE) currentHash ^= ZOBRIST_EN_PASSANT[enPassant % 8];

    // unmake move
    if (type == MoveType::QUIET) makeQuietMove(to, from);
    else if (type == MoveType::DOUBLE_PAWN_PUSH) makeQuietMove(to, from);
    else if (type == MoveType::KING_CASTLE)
    {
        if (nextMove == Color::BLACK)
        {
            makeQuietMove(Square::g1, Square::e1);
            makeQuietMove(Square::f1, Square::h1);
        }
        else
        {
            makeQuietMove(Square::g8, Square::e8);
            makeQuietMove(Square::f8, Square::h8);
        }
    }
    else if (type == MoveType::QUEEN_CASTLE)
    {
        if (nextMove == Color::BLACK)
        {
            makeQuietMove(Square::c1, Square::e1);
            makeQuietMove(Square::d1, Square::a1);
        }
        else
        {
            makeQuietMove(Square::c8, Square::e8);
            makeQuietMove(Square::d8, Square::a8);
        }
    }
    else if (type == MoveType::CAPTURE)
    {
        makeQuietMove(to, from);
        togglePiece(nextMove, pieceTaken, to);
    }
    else if (type == MoveType::EN_PASSANT)
    {
        makeQuietMove(to, from);
        if (color == Color::WHITE) togglePiece(Color::BLACK, Piece::PAWN, static_cast<Square>(to - 8));
        else togglePiece(Color::WHITE, Piece::PAWN, static_cast<Square>(to + 8));
    }
    else if (type <= MoveType::QUEEN_PROMOTION)
    {
        togglePiece(color, static_cast<Piece>(type - 4), to);
        togglePiece(color, Piece::PAWN, from);
    }
    else if (type <= MoveType::QUEEN_PROMOTION_CAPTURE)
    {
        togglePiece(color, static_cast<Piece>(type - 8), to);
        togglePiece(color, Piece::PAWN, from);
        togglePiece(nextMove, pieceTaken, to);
    }

    // update old game state
    nextMove = color;
    castlingRights = oldCastle;
    enPassant = oldEP;

    // update zobrist hash
    currentHash ^= ZOBRIST_CASTLE[castlingRights];
    if (enPassant != Square::NONE) currentHash ^= ZOBRIST_EN_PASSANT[enPassant % 8];
    currentHash ^= ZOBRIST_SIDE;
}

Square Board::makeNullMove()
{
    Square oldEP = enPassant;

    // undo old zobrists
    if (enPassant != Square::NONE) currentHash ^= ZOBRIST_EN_PASSANT[enPassant % 8];

    // update game state
    nextMove = static_cast<Color>(Color::BLACK - nextMove);
    enPassant = Square::NONE;

    // update zobrist hash
    currentHash ^= ZOBRIST_SIDE;

    return oldEP;
}

void Board::unmakeNullMove(Square oldEP)
{
    // update game state
    nextMove = static_cast<Color>(Color::BLACK - nextMove);
    enPassant = oldEP;

    // update zobrist hash
    if (enPassant != Square::NONE) currentHash ^= ZOBRIST_EN_PASSANT[enPassant % 8];
    currentHash ^= ZOBRIST_SIDE;
}

// perft with bulk counting
int Board::perft(int depth)
{
    moveGenerationSetup();
    Move moves[MAX_MOVES];
    int moveCount = 0;
    generateMoves(moves, moveCount);

    if (depth == 1) return moveCount;

    int total = 0;
    for (int i = 0; i < moveCount; i++)
    {
        Move move = moves[i];
        makeMove(move);
        total += perft(depth - 1);
        unmakeMove(move);
    }
    
    return total;
}