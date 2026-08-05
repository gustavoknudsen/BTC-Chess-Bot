#ifndef HARNESS_ENGINE_H
#define HARNESS_ENGINE_H

#include "process.h"
#include "log.h"

/*
    UCI protocol layer. One Engine owns one child process and is reused for
    every game a worker thread plays, exactly as a GUI would.
*/

#define ENGINE_OPTIONS_MAX 32
#define ENGINE_TEXT_MAX    512

enum {
    TC_TIME = 0,     // clock plus increment, optionally with moves to go
    TC_MOVETIME,     // fixed time per move
    TC_DEPTH,        // fixed depth
    TC_NODES         // fixed nodes
};

typedef struct {
    int type;
    int movesToGo;      // 0 means sudden death
    double timeMs;      // starting clock
    double incMs;
    double movetimeMs;
    int depth;
    long long nodes;
} TimeControl;

typedef struct {
    char command[ENGINE_TEXT_MAX];
    char name[128];
    char dir[ENGINE_TEXT_MAX];

    // start a new process for every game. costs a process spawn per game, and
    // buys exact reproducibility from engines whose ucinewgame leaves state
    // behind, which is most of them.
    int restartEveryGame;

    char optionNames[ENGINE_OPTIONS_MAX][64];
    char optionValues[ENGINE_OPTIONS_MAX][64];
    int  optionCount;

    TimeControl tc;
} EngineConfig;

typedef struct {
    Process proc;
    const EngineConfig *config;

    char idName[128];
    int  started;

    // result of the last engineGo
    char bestmove[16];
    int  haveScore;
    int  scoreIsMate;
    int  score;          // centipawns, or moves to mate when scoreIsMate
    int  depth;
    long long nodes;
    double elapsedMs;

    char lastError[256];
} Engine;

// when set, every command sent and every line received is written to the log.
// verbose, but it is the only way to see what an engine was actually told.
void engineSetTrace(Logger *logger);

int  engineStart(Engine *e, const EngineConfig *config);
int  engineNewGame(Engine *e);

// sends the position and go commands and waits for bestmove. hardTimeoutMs
// bounds how long the harness waits before declaring the engine unresponsive.
int  engineGo(Engine *e, const char *positionCommand, const char *goCommand, int hardTimeoutMs);

void engineQuit(Engine *e);
int  engineAlive(Engine *e);
void engineRecentOutput(Engine *e, char *out, int outSize);

#endif // HARNESS_ENGINE_H
