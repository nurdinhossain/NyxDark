#pragma once 

#include <string>
using UInt64 = unsigned long long;

enum Color
{
    WHITE,
    BLACK
};

enum Piece
{
    EMPTY,
    PAWN,
    KNIGHT,
    BISHOP,
    ROOK,
    QUEEN,
    KING
};

enum MoveType
{
    QUIET,
    DOUBLE_PAWN_PUSH,
    KING_CASTLE,
    QUEEN_CASTLE,
    CAPTURE,
    EN_PASSANT,
    KNIGHT_PROMOTION,
    BISHOP_PROMOTION,
    ROOK_PROMOTION,
    QUEEN_PROMOTION,
    KNIGHT_PROMOTION_CAPTURE,
    BISHOP_PROMOTION_CAPTURE,
    ROOK_PROMOTION_CAPTURE,
    QUEEN_PROMOTION_CAPTURE
};

enum Square 
{
    h1, g1, f1, e1, d1, c1, b1, a1,
    h2, g2, f2, e2, d2, c2, b2, a2,
    h3, g3, f3, e3, d3, c3, b3, a3,
    h4, g4, f4, e4, d4, c4, b4, a4,
    h5, g5, f5, e5, d5, c5, b5, a5,
    h6, g6, f6, e6, d6, c6, b6, a6,
    h7, g7, f7, e7, d7, c7, b7, a7,
    h8, g8, f8, e8, d8, c8, b8, a8, NONE
};

enum Castle 
{
    WHITE_KING_SIDE = 1,
    WHITE_QUEEN_SIDE = 2,
    BLACK_KING_SIDE = 4,
    BLACK_QUEEN_SIDE = 8
};

enum CastleThreat 
{
    WHITE_KINGSIDE_THREAT = 6,
    WHITE_QUEENSIDE_THREAT = 112,
    BLACK_KINGSIDE_THREAT = 432345564227567616,
    BLACK_QUEENSIDE_THREAT = 8070450532247928832
};

enum Phase
{
    PAWN_PHASE = 0,
    KNIGHT_PHASE = 1,
    BISHOP_PHASE = 1,
    ROOK_PHASE = 2,
    QUEEN_PHASE = 4,
    TOTAL_PHASE = 24
};

struct Move
{
    MoveType type;
    Square from;
    Square to;
    Piece pieceTaken;
    int oldCastle;
    Square oldEnPassant;
    int oldHistoryIndex;
    int score;
};

constexpr UInt64 FILLED_BOARD = 0xFFFFFFFFFFFFFFFFULL;
constexpr int PIECE_VALUES[5] = {100, 310, 320, 500, 900};

Color extractColor(int);
Piece extractPiece(int);
Square squareToIndex(std::string);
std::string indexToSquare(Square);

// given an index from white's perspective, return the index from whichever color's perspective
int getTableIndex(int, Color);

class Board
{
    public:
        // constructors
        Board(std::string fen);
        Board() : Board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1") {}
        ~Board();

        // clone
        Board* clone() const;

        // getters
        std::string getFen() const;
        UInt64 getPiece(Color color, Piece piece) const;
        UInt64 getOccupied(Color color) const;
        UInt64 getFullOccupied() const;
        int getSquareToPiece(Square square) const;
        int getCastlingRights() const;
        Color getNextMove() const;
        Square getEnPassant() const;
        UInt64 getCurrentHash() const;
        UInt64 getPawnHash() const;
        int getHistoryIndex() const;
        UInt64 getHistoryHash(int index) const;
        int getPieceCount(Color color, Piece piece) const;
        int getMaterial(Color color) const;
        int getStaticEvalOpening() const;
        int getStaticEvalEndgame() const;
        int getPhase() const;
        bool insufficientMaterial(Color color) const;
        UInt64 getCheckers() const;

        // setters
        void togglePiece(Color color, Piece piece, Square square);

        // other functions
        void print() const;
        void zobristHash();
        UInt64 inCheck(UInt64 bishopAttackFromKing, UInt64 rookAttackFromKing) const;
        UInt64 inCheckFull() const;
        UInt64 getAttackersForSquare(Square square) const;
        UInt64 checkBlockMask(UInt64 bishopAttackFromKing, UInt64 rookAttackFromKing) const;
        UInt64 pinned(UInt64 bishopAttackFromKing, UInt64 rookAttackFromKing) const;
        UInt64 pinnedLegalMask(Square pinnedIndex) const;

        // move generation
        UInt64 pawnPushBoard(Square square) const;
        UInt64 pawnAttackBoard(Square square) const;
        UInt64 knightAttackBoard(Square square) const;
        UInt64 bishopAttackBoard(Square square) const;
        UInt64 rookAttackBoard(Square square) const;
        UInt64 queenAttackBoard(Square square) const;
        UInt64 kingAttackBoard(Square square) const;
        void moveGenerationSetup();
        void generateMoves(Move moves[], int& moveCount, bool attackOnly = false, bool quietOnly = false);
        bool isEPLegal(Square from, Square to);
        bool isKingMoveLegal(Square from, Square to);
        void makeQuietMove(Square from, Square to);
        void makeCaptureMove(Square from, Square to);
        void makeEPMove(Square from, Square to);
        void castleKingSide(Color color);
        void castleQueenSide(Color color);
        void makePromoMove(Square from, Square to, Piece promoPiece);
        void makePromoCaptureMove(Square from, Square to, Piece promoPiece);
        void makeMove(Move &move);
        void unmakeMove(Move &move);
        Square makeNullMove();
        void unmakeNullMove(Square oldEnPassant);
        int perft(int depth);
        
    private:
        // for board state
        UInt64 pieces[2][6] = {0}; // 2 colors, 6 pieces
        UInt64 occupied[2] = {0};; // 2 colors
        UInt64 fullOccupied = 0; // all occupied squares
        int squareToPiece[64] = {0}; // maps square to piece

        // for move generation
        int castlingRights = 0; // 4 bits for white king, white queen, black king, black queen
        Color nextMove = WHITE; // 0 for white, 1 for black
        Square enPassant = NONE; // 0-63 for square, 64 for none

        // hashing
        UInt64 currentHash = 0; // current hash
        UInt64 pawnHash = 0; // hash for pawns
        UInt64 history[1024] = {0}; // history for hashing
        int historyIndex = 0; // index for history

        // for generating legal moves
        Square kingIndices[2] = {NONE, NONE}; // 2 colors
        UInt64 pinnedPiecesMG = 0; // pinned pieces for move generation
        UInt64 checkers = 0; // checkers for move generation
        UInt64 occMG = 0; // occupancy for move generation  
        UInt64 blockMaskMG = 0; // block mask for move generation

        // for static evaluation
        int pieceCounts[2][6] = {0}; // 2 colors, 6 pieces
        int material[2] = {0}; // 2 colors
        int staticEvalOpening = 0; // static evaluation for opening
        int staticEvalEndgame = 0; // static evaluation for endgame
};
