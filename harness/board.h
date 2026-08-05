#ifndef HARNESS_BOARD_H
#define HARNESS_BOARD_H

/*
    Arbiter board.

    This is deliberately independent of the engine in src/. The arbiter must
    not share a move generator with the engine it is judging, and it must be
    re-entrant so several games can run in parallel threads. Speed does not
    matter here: a whole game costs a few thousand nodes.

    Squares are a1 = 0 ... h8 = 63 (rank-major, file within rank).
*/

// piece codes, 0 means an empty square
enum {
    EMPTY = 0,
    WP, WN, WB, WR, WQ, WK,
    BP, BN, BB, BR, BQ, BK,
    PIECE_NB
};

enum { WHITE = 0, BLACK = 1 };

// castling right bits
enum {
    CR_WK = 1,
    CR_WQ = 2,
    CR_BK = 4,
    CR_BQ = 8
};

// move flags
enum {
    MF_CAPTURE = 1,
    MF_DOUBLE  = 2,
    MF_EP      = 4,
    MF_CASTLE  = 8
};

typedef struct {
    unsigned char from;
    unsigned char to;
    unsigned char promo;   // promoted piece code, 0 when not a promotion
    unsigned char flags;
} Move;

#define MAX_MOVES 256

typedef struct {
    unsigned char squares[64];
    unsigned char side;
    unsigned char castling;
    signed char   ep;        // en passant target square, -1 when none
    int halfmove;            // plies since the last capture or pawn move
    int fullmove;
    unsigned long long key;
} Position;

// game termination values
enum {
    RES_NONE = 0,
    RES_WHITE_WINS,
    RES_BLACK_WINS,
    RES_DRAW
};

// call once at startup, before any other board function
void boardInit(void);

int  boardParseFen(Position *pos, const char *fen);
void boardWriteFen(const Position *pos, char *out, int outSize);

int  boardGenerateLegal(const Position *pos, Move *list);
void boardMakeMove(Position *pos, Move m);
int  boardInCheck(const Position *pos, int side);

int  boardMoveFromUci(const Position *pos, const char *text, Move *out);
void boardMoveToUci(Move m, char *out);
int  boardMoveFromSan(const Position *pos, const char *text, Move *out);
void boardMoveToSan(const Position *pos, Move m, char *out, int outSize);

int  boardInsufficientMaterial(const Position *pos);

unsigned long long boardPerft(const Position *pos, int depth);

#endif // HARNESS_BOARD_H
