#ifndef HARNESS_GAME_H
#define HARNESS_GAME_H

#include "board.h"
#include "book.h"
#include "engine.h"
#include "log.h"

#define GAME_MOVES_MAX 600

typedef struct {
    // resign adjudication: both engines must agree the game is decided for
    // this many moves in a row
    int resignEnabled;
    int resignMoveCount;
    int resignScore;

    // draw adjudication: from this move number on, both engines must report a
    // score inside the window for this many moves in a row
    int drawEnabled;
    int drawMoveNumber;
    int drawMoveCount;
    int drawScore;

    int maxMoves;        // 0 means no cap
    int timeMarginMs;    // grace before a clock overrun counts as a loss
} GameRules;

typedef struct {
    char uci[6];
    char san[16];
    int  haveScore;
    int  scoreIsMate;
    int  score;          // from the point of view of the side that moved
    int  depth;
    double timeMs;
} GameMove;

enum {
    TERM_NORMAL = 0,       // decided on the board
    TERM_ADJUDICATION,
    TERM_TIME_LOSS,
    TERM_ILLEGAL_MOVE,
    TERM_ENGINE_FAILURE
};

typedef struct {
    char startFen[128];
    int  openingPlies;   // leading moves that came from the book

    GameMove moves[GAME_MOVES_MAX];
    int moveCount;

    int  result;         // RES_WHITE_WINS, RES_BLACK_WINS or RES_DRAW
    char reason[160];    // termination text for the PGN
    int  termination;

    // set when an engine has to be replaced before the next game
    int restartWhite;
    int restartBlack;
} GameRecord;

/*
    Plays one complete game and fills the record. Returns 1 when the game was
    played to a result and 0 only if it could not be started at all, which
    means a broken opening or an engine that was already gone.
*/
int playGame(Engine *white, Engine *black, const Opening *opening,
             const GameRules *rules, GameRecord *record, Logger *logger,
             const char *label);

#endif // HARNESS_GAME_H
